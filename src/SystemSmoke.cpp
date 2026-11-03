#include "SystemSmoke.h"
#include "VehicleState.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QDir>
#include <QTimer>
#include <QtTest>
#include <functional>
#include <stdexcept>

void runSystemSmoke(QGuiApplication &app,QQuickWindow &window,VehicleState &vehicle,QStringList &warnings){
    auto *host=vehicle.systemStatus();host->setSamplingEnabled(false);
    host->applyPowerSample({true,0,64,0,0,{}});
    host->applyNetworkSample({"ethernet","online","test fixture",{QStringLiteral("测试有线适配器")},false,false,true});
    window.setProperty("testMode",true);window.setProperty("clockText","16:28");
    QTimer::singleShot(1200,&app,[&app,&window,&vehicle,&warnings,host]{
        const QString output=qEnvironmentVariable("CAR_TERMINAL_SYSTEM_OUTPUT","artifacts/system/ui");QDir().mkpath(output);window.raise();window.requestActivate();
        auto require=[](bool ok,const char *reason){if(!ok)throw std::runtime_error(reason);};
        std::function<QQuickItem*(QQuickItem*,const QString&)> find;find=[&](QQuickItem *parent,const QString &name)->QQuickItem*{if(parent->objectName()==name&&parent->isVisible())return parent;for(auto *child:parent->childItems())if(auto *hit=find(child,name))return hit;return nullptr;};
        auto item=[&](const char *name){auto *hit=find(window.contentItem(),name);require(hit,name);return hit;};
        auto wait=[&](std::function<bool()> test,const char *reason){for(int i=0;i<300&&!test();++i)QTest::qWait(20);require(test(),reason);};
        auto click=[&](const char *name){auto *control=item(name);require(control->isEnabled(),name);QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,control->mapToScene(QPointF(control->width()/2,control->height()/2)).toPoint());QTest::qWait(100);};
        auto capture=[&](const QString &name){window.setProperty("toastMessage","");window.setProperty("feedbackQueue",QVariantList{});QTest::qWait(300);require(window.grabWindow().save(output+"/"+name+".png"),"save system UI capture");};
        try{
            wait([&]{return item("homeVehicleScene")->property("ready").toBool();},"home renderer ready");click("sourceButton");click("sourceSystem");require(vehicle.systemMode()&&vehicle.batteryPercent()==64,"select computer source");capture("01-source-system");click("closePopup");
            require(item("statusBattery")->property("text")=="64%"&&item("homeBatteryValue")->property("text")=="64","shared real SOC labels");capture("02-battery-power");
            click("homeNearbyStations");require(item("nearbyChargeRange")->property("text").toString().startsWith(QStringLiteral("估算续航 ")),"nearby range identifies Windows energy estimate");capture("02b-nearby-system");click("closePopup");
            vehicle.startDriving();vehicle.advanceSimulation(60);require(vehicle.batteryPercent()==64,"real SOC does not discharge virtually");vehicle.parkVehicle();host->applyPowerSample({true,8,65,1,0,{}});wait([&]{return vehicle.charging();},"system charging status");
            click("statusEnergyButton");require(item("statusEnergyIndicator")->property("name")=="bolt","actual charging status indicator");require(item("systemChargeReadOnly"),"read-only charging panel");require(!find(window.contentItem(),"chargeStart")&&!find(window.contentItem(),"chargeTargetSlider")&&!find(window.contentItem(),"chargeFastForward"),"no unsupported charge controls");require(item("chargeSocLabel")->property("text")=="65%","system SOC precision");capture("03-system-charging");
            const auto before=vehicle.batteryPercent();vehicle.advanceSimulation(3600);vehicle.stopCharging();vehicle.setChargeTarget(100);require(vehicle.batteryPercent()==before&&vehicle.charging(),"UI cannot override actual charging");
            host->applyPowerSample({true,0,92,1,0,{}});require(!vehicle.charging()&&vehicle.plugged(),"AC without charging");capture("04-ac-not-charging");
            host->applyPowerSample({});require(!vehicle.energyReady()&&vehicle.batteryPercent()==92&&!vehicle.systemFallback(),"power failure retains last SOC");capture("05-power-unknown");
            host->applyPowerSample({true,128,255,1,0,{}});require(vehicle.systemFallback()&&vehicle.batteryPercent()==82&&!vehicle.plugged(),"desktop fallback");require(item("chargeStart"),"desktop simulation controls restored");capture("06-desktop-fallback");
            host->applyPowerSample({true,0,55,0,0,{}});require(!vehicle.systemFallback()&&vehicle.batteryPercent()==55,"battery reappears");click("tab0");
            for(const auto &type:QStringList{"wifi","none","unknown","mixed"}){
                host->applyNetworkSample({type,type=="none"?"offline":type=="unknown"?"unknown":"local","test fixture",{},false,false,false});
                require(item("networkStatusButton")->property("iconName")==host->network()["icon"],"network type drives icon");capture("07-network-"+type);
            }
            click("networkStatusButton");require(window.property("popup")=="system","network details entry");require(item("systemNetworkState")->property("text")==host->network()["summary"],"network detail truth");capture("08-system-details");click("closePopup");
            vehicle.setSourceMode("internal");require(vehicle.batteryPercent()==82,"internal SOC restored");host->applyPowerSample({});vehicle.setSourceMode("system");require(!vehicle.batteryKnown()&&!vehicle.energyReady(),"initial unavailable power remains unknown");require(item("statusBattery")->property("text")==QString::fromUtf8("—"),"unknown is not zero or fake percentage");capture("09-initial-unknown");
            click("notificationsButton");require(item("notificationEnergy")->property("text").toString().endsWith(QStringLiteral("估算续航 未提供")),"notifications do not show simulated range for unknown battery");click("closePopup");vehicle.setSourceMode("internal");
            require(warnings.isEmpty(),qPrintable(warnings.join('\n')));qInfo()<<"PASS system UI: three sources, real SOC precision, read-only power, desktop fallback, unknown data, network types and details";app.exit(0);
        }catch(const std::exception &error){window.grabWindow().save(output+"/failure.png");qCritical()<<"SYSTEM UI FAIL:"<<error.what();app.exit(2);}
    });
}

#include <QtTest>
#include "VehicleState.h"

class VehicleSystemTests:public QObject {
    Q_OBJECT
    static PowerSample battery(int percent,int flags=0,int ac=0){return {true,flags,percent,ac,0,{}};}
private slots:
    void initTestCase(){qputenv("CAR_TERMINAL_NO_SETTINGS","1");}
    void unknownFlagsAreNotNoBattery(){
        auto p=SystemStatus::normalize(battery(255,255,255));QVERIFY(!p.presenceKnown);QVERIFY(!p.socKnown);QVERIFY(!p.valid);
        p=SystemStatus::normalize(battery(255,128,1));QVERIFY(p.presenceKnown);QVERIFY(!p.hasBattery);QVERIFY(!p.socKnown);
        p=SystemStatus::normalize(battery(255));QVERIFY(p.hasBattery);QVERIFY(!p.valid);
        p=SystemStatus::normalize(battery(64,8,1));QVERIFY(p.valid);QVERIFY(p.charging);
        p=SystemStatus::normalize(battery(64,8,0));QVERIFY(!p.valid);
    }
    void actualEnergyIsReadOnlyAndRestoresLocal(){
        SystemStatus host(nullptr,false);host.applyPowerSample(battery(64));VehicleState v(nullptr,&host);const auto initial=v.batteryPercent();
        v.setSourceMode("system");QVERIFY(v.systemMode());QVERIFY(v.energyReady());QCOMPARE(v.batteryPercent(),64.0);QVERIFY(!v.chargeControlAvailable());QVERIFY(!v.plugged());
        v.startDriving();QCOMPARE(v.speed(),40);v.advanceSimulation(300);QCOMPARE(v.batteryPercent(),64.0);QVERIFY(v.tripKm()>3);
        v.setSourceMode("internal");QVERIFY(v.systemMode());v.parkVehicle();
        host.applyPowerSample(battery(64,8,1));QVERIFY(v.charging());QVERIFY(v.plugged());QVERIFY(v.portOpen());QVERIFY(!v.chargePowerKnown());QCOMPARE(v.chargePower(),0.0);QCOMPARE(v.chargeMinutes(),0);
        const auto target=v.chargeTarget();v.setChargeTarget(100);v.stopCharging();v.togglePlug();v.togglePort();v.advanceSimulation(3600);
        QCOMPARE(v.chargeTarget(),target);QVERIFY(v.charging());QVERIFY(v.plugged());QCOMPARE(v.batteryPercent(),64.0);
        host.applyPowerSample(battery(92,0,1));QVERIFY(!v.charging());QVERIFY(v.plugged());QCOMPARE(v.batteryPercent(),92.0);
        v.setVolume(51);v.setSourceMode("internal");QCOMPARE(v.batteryPercent(),initial);QCOMPARE(v.volume(),51);QVERIFY(!v.plugged());QVERIFY(v.chargeControlAvailable());
    }
    void noBatteryFallbackAndLaterDetection(){
        SystemStatus host(nullptr,false);host.applyPowerSample(battery(255,128,1));VehicleState v(nullptr,&host);
        v.setSourceMode("system");QVERIFY(v.systemFallback());QVERIFY(v.energyReady());QVERIFY(v.batteryKnown());QVERIFY(!v.plugged());QCOMPARE(v.batteryPercent(),82.0);
        v.startDriving();v.advanceSimulation(60);QVERIFY(v.batteryPercent()<82);const auto simulated=v.batteryPercent();v.parkVehicle();
        host.applyPowerSample(battery(45));QVERIFY(!v.systemFallback());QCOMPARE(v.batteryPercent(),45.0);QVERIFY(!v.chargeControlAvailable());
        host.applyPowerSample(battery(255,128,1));QVERIFY(v.systemFallback());QCOMPARE(v.batteryPercent(),simulated);QVERIFY(!v.plugged());
    }
    void batteryRemovalPausesDrivingBeforeRestoringFallback(){
        SystemStatus host(nullptr,false);host.applyPowerSample(battery(45));VehicleState v(nullptr,&host);
        v.togglePort();v.togglePlug();v.startCharging();QVERIFY(v.charging());
        v.setSourceMode("system");v.startDriving();QCOMPARE(v.speed(),40);
        host.applyPowerSample(battery(255,128,1));QVERIFY(v.systemFallback());QCOMPARE(v.speed(),0);QCOMPARE(v.gear(),QString("D"));
        QVERIFY(v.charging());QVERIFY(v.plugged());v.setSpeed(40);QCOMPARE(v.speed(),0);
    }
    void failuresPreserveValuesAndDoNotStartFallback(){
        SystemStatus host(nullptr,false);host.applyPowerSample({});VehicleState v(nullptr,&host);v.setSourceMode("system");QVERIFY(!v.batteryKnown());QVERIFY(!v.energyReady());QVERIFY(!v.systemFallback());
        host.applyPowerSample(battery(61));QVERIFY(v.energyReady());QCOMPARE(v.batteryPercent(),61.0);v.startDriving();QCOMPARE(v.gear(),QString("D"));
        host.applyPowerSample({});QVERIFY(!v.energyReady());QVERIFY(v.batteryKnown());QCOMPARE(v.batteryPercent(),61.0);QCOMPARE(v.speed(),0);QCOMPARE(v.gear(),QString("D"));QVERIFY(!v.systemFallback());
        v.setSpeed(40);QCOMPARE(v.speed(),0);host.applyPowerSample(battery(60));QVERIFY(v.energyReady());QCOMPARE(v.batteryPercent(),60.0);
        v.setSpeed(40);QCOMPARE(v.speed(),40);host.applyPowerSample(battery(60,8,1));QCOMPARE(v.speed(),0);QCOMPARE(v.gear(),QString("D"));QVERIFY(v.charging());v.setSpeed(40);QCOMPARE(v.speed(),0);
    }
    void networkStatesAndDomainsAreIndependent(){
        SystemStatus host(nullptr,false);VehicleState v(nullptr,&host);const auto soc=v.batteryPercent();
        for(const auto &type:QStringList{"ethernet","wifi","cellular","mixed","unknown","none"}){
            host.applyNetworkSample({type,type=="none"?"offline":"online","fixture",{},false,false,false});QCOMPARE(host.network()["transport"].toString(),type);QCOMPARE(v.batteryPercent(),soc);
        }
        host.applyNetworkSample({"wifi","local","fixture",{},true,true,true});QVERIFY(host.network()["captive"].toBool());QVERIFY(host.network()["metered"].toBool());
        host.applyPowerSample(battery(30));v.setSourceMode("external");QCOMPARE(v.sourceMode(),QString("external"));host.applyPowerSample(battery(15));QCOMPARE(v.batteryPercent(),soc);v.setSourceMode("system");QCOMPARE(v.batteryPercent(),15.0);
    }
};
QTEST_GUILESS_MAIN(VehicleSystemTests)
#include "VehicleSystemTests.moc"

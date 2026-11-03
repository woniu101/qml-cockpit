#include "UiSmoke.h"
#include "VehicleState.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QMediaPlayer>
#include <QDir>
#include <QTimer>
#include <QWheelEvent>
#include <QPropertyAnimation>
#include <QJsonDocument>
#include <QFile>
#include <QScreen>
#include <QVector3D>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif
#include <QtTest>
#include <functional>
#include <stdexcept>
#include <cmath>

void runUiSmoke(QGuiApplication &app,QQuickWindow &window,VehicleState &state,QMediaPlayer &player,QStringList &warnings,bool native) {
    window.setProperty("testMode",true);window.setProperty("clockText","16:28");window.setProperty("dateText",QStringLiteral("11月2日 周一"));window.setProperty("greetingText",QStringLiteral("下午好，林先生"));window.setProperty("arrivalText","16:46");
    QTimer::singleShot(1800,&app,[&,native]{
        window.raise();window.requestActivate();const QString directory=QDir::current().absoluteFilePath(qEnvironmentVariable("CAR_TERMINAL_SMOKE_DIR",native?"artifacts/v6/native":"artifacts/v6/basic"));QDir().mkpath(directory);
        auto require=[](bool value,const char *message){if(!value)throw std::runtime_error(message);};
        std::function<QQuickItem*(QQuickItem*,const QString&)> find;
        find=[&](QQuickItem *parent,const QString &name)->QQuickItem*{if(parent->objectName()==name&&parent->isVisible())return parent;for(auto *child:parent->childItems())if(auto *item=find(child,name))return item;return nullptr;};
        auto item=[&](const char *name){auto *value=find(window.contentItem(),QString::fromLatin1(name));require(value,name);return value;};
        auto wait=[&](std::function<bool()> condition,const char *message,int timeout=4500){for(int n=0;!condition()&&n<timeout/25;++n)QTest::qWait(25);require(condition(),message);};
        auto idle=[&]{wait([&]{return !state.busy();},"motion completion");};
        auto click=[&](const char *name,double fraction=0.5){
            auto *control=item(name);wait([&]{return control->isEnabled();},name);
            QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,control->mapToScene(QPointF(control->width()*fraction,control->height()/2)).toPoint());QTest::qWait(110);
        };
        auto capture=[&](const QString &name,bool clear=true){if(clear)window.setProperty("toastMessage","");QTest::qWait(clear?230:100);const auto image=window.grabWindow();require(!image.isNull(),"empty capture");require(image.save(directory+"/"+name+".png"),"save capture");return image;};
        auto differentPixels=[&](const QImage &a,const QImage &b,QQuickItem *region){
            const double scale=double(a.width())/window.width();const QRectF rect=region->mapRectToScene(region->boundingRect());
            const QRect roi=QRect(qRound(rect.x()*scale),qRound(rect.y()*scale),qRound(rect.width()*scale),qRound(rect.height()*scale)).intersected(a.rect());
            int changed=0;for(int y=roi.top();y<=roi.bottom();++y)for(int x=roi.left();x<=roi.right();++x){const auto p=a.pixelColor(x,y),q=b.pixelColor(x,y);if(std::abs(p.red()-q.red())+std::abs(p.green()-q.green())+std::abs(p.blue()-q.blue())>30)++changed;}
            return changed;
        };
        try {
            wait([&]{return item("homeVehicleScene")->property("ready").toBool();},"3D home ready",8000);capture("01-home");
            click("tab2");auto *scene=item("controlsVehicleScene");wait([&]{return scene->property("ready").toBool();},"3D controls ready");
            QTest::qWait(500);const auto closed=capture("02-window-closed");
            click("windowOpeningSlider",0.35);require(std::abs(state.windowActual()[0].toDouble()-35)<3,"actual UI percentage slider");require(!state.windowBusy()[0].toBool(),"drag release confirms without stale motion");
            click("windowPercentText");QTest::keyClick(&window,Qt::Key_A,Qt::ControlModifier);QTest::keyClick(&window,Qt::Key_3);QTest::keyClick(&window,Qt::Key_5);QTest::keyClick(&window,Qt::Key_Return);idle();
            require(state.windowActual()[0].toDouble()==35,"exact 35 percent from editable UI");
            require(std::abs(scene->property("windowLF").toDouble()-0.35)<0.001,"3D and UI share actual opening");const auto open35=capture("03-window-35");
            require(differentPixels(closed,open35,scene)>200,"actual glass mesh changes on screen");
            auto *touch=QTest::createTouchDevice();const double initialDistance=scene->property("distance").toDouble();
            const QPoint left=scene->mapToScene(QPointF(320,235)).toPoint(),right=scene->mapToScene(QPointF(520,235)).toPoint();
            auto gesture=QTest::touchEvent(&window,touch,false);gesture.press(0,left,&window).press(1,right,&window).commit();QTest::qWait(60);
            gesture.move(0,left-QPoint(35,0),&window).move(1,right+QPoint(35,0),&window).commit();QTest::qWait(60);
            gesture.move(0,left-QPoint(75,0),&window).move(1,right+QPoint(75,0),&window).commit();QTest::qWait(60);
            gesture.release(0,left-QPoint(75,0),&window).release(1,right+QPoint(75,0),&window).commit();
            require(scene->property("distance").toDouble()<initialDistance-30,"two finger pinch zoom");
            click("resetVehicleView");QTest::qWait(500);
            click("windowPreset100");require(state.windowBusy()[0].toBool(),"preset starts continuous movement");QTest::qWait(250);click("stopWindow");
            const double stopped=state.windowActual()[0].toDouble();require(stopped>35&&stopped<100,"stop midway");QTest::qWait(180);require(state.windowActual()[0].toDouble()==stopped,"stopped opening stable");capture("04-window-stopped");
            click("windowSelect1");click("windowPreset50");idle();require(state.windows()[1].toInt()==50,"right front half open");
            click("windowSelect3");click("windowPreset15");idle();require(state.windows()[3].toInt()==15,"rear vent preset");
            click("windowPreset100");idle();require(item("selectedPartMarker"),"window frame marker remains after fully lowered glass");capture("05a-window-full-open");
            click("notificationsButton");require(item("notification_window1"),"window notification");capture("05-window-notifications");click("inspectNotification_window1");
            require(scene->property("selectedPart").toString()=="window1","notification selects correct window");QTest::qWait(500);capture("06-focus-right");
            click("closeAllWindows");auto *banner=item("feedbackBanner");require(banner->y()>=80&&banner->y()+banner->height()<=150,"feedback reserved below status bar");capture("07-batch-progress",false);idle();
            require(state.windows()[0].toInt()==0&&state.windows()[1].toInt()==0&&state.windows()[3].toInt()==0,"batch closes all actual openings");
            click("doorMode");click("toggleSelectedDoor");idle();require(state.doors()[1].toBool(),"named door action");capture("08-door-open");
            click("tab0");click("destination1");require(state.navigationActive()&&state.parked()&&state.speed()==0,"route planning preserves parking even with open door");capture("09-route-parked");
            click("simulatorButton");click("startDriving");require(state.parked(),"open door blocks driving, not planning");capture("10-driving-warning",false);click("closePopup");
            click("tab2");click("closeAllDoors");idle();click("simulatorButton");click("startDriving");require(state.gear()=="D"&&state.speed()==40&&window.property("page").toInt()==4&&window.property("popup").toString().isEmpty(),"start independent driving and close modal");click("drivePageFastForward");require(state.routeProgress()>0,"driving advances actual selected route");click("drivePark");require(state.navigationActive()&&state.parked(),"park retains route");
            click("tab1");capture("11-route-progress");click("endNavigation");require(state.parked()&&!state.navigationActive(),"navigation can end independently");
            click("tab2");click("vehicleSection0");const auto mirrorsOpen=capture("12-quick-controls");click("quick_mirrors");QTest::qWait(750);require(state.mirrorsFolded(),"mirror fold state");const auto mirrorsFolded=capture("13-mirrors-folded");require(differentPixels(mirrorsOpen,mirrorsFolded,scene)>50,"mirror mesh actually folds");click("quick_mirrors");
            click("vehicleSection5");QTest::qWait(550);const auto portClosed=capture("14a-port-closed");click("chargePort");QTest::qWait(650);const auto portOpen=capture("14-port-open");require(differentPixels(portClosed,portOpen,scene)>40,"charge port mesh opens on visible side");click("chargePlug");click("chargeStart");require(state.charging(),"plug and charge workflow");const double battery=state.batteryPercent();click("chargeFastForward");require(state.batteryPercent()>battery,"charge changes SOC and range");const auto charging=capture("15-charging");require(differentPixels(portOpen,charging,scene)>80,"charge connector visible on mesh");
            if(state.charging())click("chargeStart");click("chargePlug");click("chargePort");require(!state.plugged()&&!state.portOpen(),"disconnect and close charge port");
            click("vehicleSection3");click("highBeam");require(state.highBeam(),"light group");capture("16-lights");
            click("climateButton");click("climateSync");click("driverTemperatureSlider",0.4);require(state.climateSync()&&state.driverTemperature()==state.passengerTemperature(),"dual climate sync");capture("17-climate");click("closePopup");
            click("tab3");click("fullMusicPlay");require(state.playing(),"media play");wait([&]{return player.duration()>1000;},"local audio decode");click("trackRow0");require(!state.playing(),"current playlist row pauses");
            const auto queue=state.playQueue();require(queue.size()==3,"current view defines built-in queue");const auto nextUrl=queue[(queue.indexOf(state.trackUrl())+1)%queue.size()];click("fullMusicNext");require(state.trackUrl()==nextUrl&&state.playing(),"next song follows visible queue order");wait([&]{return player.source()==QUrl(nextUrl)&&player.duration()>1000;},"queued audio decoded");state.seek(15000);QTest::qWait(250);require(player.position()>14000,"media seek");capture("18-music");
            click("settingsButton");click("theme_night");require(state.night(),"theme preference");require(item("settingsTitle")->property("color").value<QColor>().lightness()>150,"night text remains readable");capture("19-settings-night");click("theme_day");click("closePopup");
            const auto before=state.snapshot();window.setProperty("page",0);window.setProperty("popup","demo");QTest::qWait(100);click("autoDemoButton");require(window.property("demoRunning").toBool(),"start controlled demo");click("demoPauseInline");require(window.property("demoPaused").toBool(),"demo pause");click("demoNextInline");require(window.property("demoStep").toInt()==1,"single step");click("demoStopInline");require(!window.property("demoRunning").toBool()&&state.driverTemperature()==before["driver"].toDouble()&&state.volume()==before["volume"].toInt(),"exit demo restores preferences and runtime");
            if(native&&!qEnvironmentVariableIsSet("CAR_TERMINAL_SKIP_SOAK")){
                for(int cycle=0;cycle<3;++cycle){
                    window.setProperty("popup","demo");QTest::qWait(80);click("autoDemoButton");click("demoPauseInline");
                    for(int step=1;step<12;++step){click("demoNextInline");QTest::qWait(1000);require(window.property("demoRunning").toBool()&&window.property("demoStep").toInt()==step,"repeated demo reaches each step");}
                    click("demoNextInline");require(!window.property("demoRunning").toBool(),"complete demo restores state");require(state.volume()==before["volume"].toInt()&&state.batteryPercent()==before["battery"].toDouble(),"repeated demo restores battery and preferences");
                }
            }
            window.resize(1120,700);QTest::qWait(200);click("tab2");click("windowOpeningSlider",0.5);require(std::abs(state.windowActual()[0].toDouble()-50)<3,"scaled pointer percentage");capture("20-scaled");
            window.resize(1120,630);QTest::qWait(200);click("windowOpeningSlider",0.35);require(std::abs(state.windowActual()[0].toDouble()-35)<3,"minimum window pointer percentage");capture("20b-minimum");
            scene->setProperty("renderFailed",true);capture("21-fallback");click("reloadVehicle3D");wait([&]{return scene->property("ready").toBool();},"3D retry recovers from fallback",8000);
            const QSize screenSpace=window.screen()->availableGeometry().size()-QSize(32,64);window.resize(std::min(1600,screenSpace.width()),std::min(900,screenSpace.height()));QTest::qWait(160);
            click("tab0");click("homeDriving");auto *drive=item("drivingVehicleScene");wait([&]{return drive->property("ready").toBool();},"driving 3D ready",8000);
            require(state.gear()=="D"&&state.speed()==40&&!state.navigationActive(),"free driving without creating a route");QTest::qWait(550);
            const auto movingA=capture("22-driving-free");const double phaseA=drive->property("travelMeters").toDouble();QTest::qWait(300);const auto movingB=capture("23-driving-motion");
            require(drive->property("travelMeters").toDouble()>phaseA+1,"road advances with speed");require(differentPixels(movingA,movingB,drive)>400,"driving changes actual scene pixels");
            auto *wheel=drive->findChild<QObject*>("wheels_wheels.2_0");require(wheel,"real tire model present");const float angle=wheel->property("eulerRotation").value<QVector3D>().x();require(std::abs(angle)>1,"wheel rotation is bound to moving tire mesh");
            const double trip=state.tripKm();click("drivePageFastForward");require(state.tripKm()>trip+0.6,"trip distance advances without navigation");
            click("drivePageSpeed",0.7);require(state.speed()>65&&state.speed()<75,"visible speed adjustment");click("driveGearR");require(state.speed()<=15,"reverse speed limit");const double forwardPhase=drive->property("travelMeters").toDouble();QTest::qWait(250);require(drive->property("travelMeters").toDouble()<forwardPhase,"reverse animates road and wheels backwards");
            click("drivePark");const double parkedPhase=drive->property("travelMeters").toDouble();QTest::qWait(220);require(drive->property("travelMeters").toDouble()==parkedPhase&&state.speed()==0,"P stops wheel and road motion");capture("24-driving-parked");
            click("driveNavigation");click("searchDestination1");require(state.navigationActive()&&state.parked(),"planning retains P from driving view");click("simulatorButton");click("startDriving");require(window.property("page").toInt()==4&&state.navigationActive(),"driving view reuses planned route");click("drivePageFastForward");require(state.routeProgress()>0,"route progresses in driving view");capture("25-driving-route");click("drivePark");click("driveNavigation");click("endNavigation");
            click("tab0");const double clockCenter=item("statusClock")->mapToScene(QPointF(0,item("statusClock")->height()/2)).y();const double batteryCenter=item("statusBattery")->mapToScene(QPointF(0,item("statusBattery")->height()/2)).y();require(std::abs(clockCenter-batteryCenter)<0.1,"status groups share vertical center");capture("26-status-home");
            if(native){
                click("tab2");
                const QSize available=window.screen()->availableGeometry().size()-QSize(32,64);
                window.resize(std::min(1600,available.width()),std::min(900,available.height()));QTest::qWait(300);click("vehicleSection0");scene->setProperty("selectedPart","");
                QVariantMap metrics{{"window",QString("%1x%2").arg(window.width()).arg(window.height())},{"devicePixelRatio",window.devicePixelRatio()},{"logicalDpi",window.screen()->logicalDotsPerInch()},{"renderLoop",qEnvironmentVariable("QSG_RENDER_LOOP","default")}};
                for(const auto &quality:QStringList{"standard","high"}){
                    state.setQuality(quality);QTest::qWait(700);QVector<double> intervals;QElapsedTimer timer;timer.start();qint64 previous=0;
                    const auto connection=QObject::connect(&window,&QQuickWindow::frameSwapped,&window,[&]{const qint64 at=timer.nsecsElapsed();if(previous)intervals.append(double(at-previous)/1000000);previous=at;},Qt::QueuedConnection);
                    QPropertyAnimation orbit(scene,"yaw");orbit.setStartValue(scene->property("yaw"));orbit.setEndValue(scene->property("yaw").toDouble()+180);orbit.setDuration(4000);orbit.start();QTest::qWait(4050);QObject::disconnect(connection);
                    require(intervals.size()>10,"native renderer presents animation frames");std::sort(intervals.begin(),intervals.end());double total=0;for(double value:intervals)total+=value;
                    metrics[quality]=QVariantMap{{"frames",intervals.size()+1},{"averageFps",1000.0*intervals.size()/total},{"p95FrameMs",intervals[qMin(intervals.size()-1,qsizetype(intervals.size()*0.95))]},{"durationMs",total}};
                }
#ifdef Q_OS_WIN
                PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);if(K32GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory))){metrics["workingSetMiB"]=double(memory.WorkingSetSize)/1048576;metrics["peakWorkingSetMiB"]=double(memory.PeakWorkingSetSize)/1048576;metrics["privateMiB"]=double(memory.PrivateUsage)/1048576;}
#endif
                state.setQuality("standard");click("tab0");click("homeDriving");QTest::qWait(700);
                QElapsedTimer driveTimer;driveTimer.start();QVector<double> driveIntervals;qint64 drivePrevious=0;
                const auto driveConnection=QObject::connect(&window,&QQuickWindow::frameSwapped,&window,[&]{const qint64 at=driveTimer.nsecsElapsed();if(drivePrevious)driveIntervals.append(double(at-drivePrevious)/1000000);drivePrevious=at;},Qt::QueuedConnection);
                QTest::qWait(3100);QObject::disconnect(driveConnection);require(driveIntervals.size()>10,"driving page continuously presents motion frames");std::sort(driveIntervals.begin(),driveIntervals.end());double driveTotal=0;for(double value:driveIntervals)driveTotal+=value;
                metrics["drivingStandard"]=QVariantMap{{"averageFps",1000.0*driveIntervals.size()/driveTotal},{"p95FrameMs",driveIntervals[qMin(driveIntervals.size()-1,qsizetype(driveIntervals.size()*0.95))]}};capture("27-driving-final");click("drivePark");
                QFile output(directory+"/performance.json");require(output.open(QIODevice::WriteOnly),"write performance report");output.write(QJsonDocument::fromVariant(metrics).toJson());output.close();
                qInfo().noquote()<<"Native performance:"<<QJsonDocument::fromVariant(metrics).toJson(QJsonDocument::Compact);
            }
            require(warnings.isEmpty(),qPrintable(warnings.join('\n')));
            qInfo()<<"PASS v6: aligned status groups, free driving and route driving, real wheel and road movement, P stop, reverse, trip distance, plus v5 interaction regression";
            app.exit(0);
        }catch(const std::exception &error){
            if(auto *scene=find(window.contentItem(),"controlsVehicleScene"))qCritical()<<"Scene diagnostics"<<scene->property("activeView")<<scene->property("presentedFrames")<<scene->property("renderFailed");
            window.grabWindow().save(directory+"/failure.png");qCritical()<<"UI smoke FAIL:"<<error.what();app.exit(2);
        }
    });
}

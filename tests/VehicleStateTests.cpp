#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include <QUuid>
#include "VehicleState.h"
#include "assistant/AssistantVehicle.h"
#include "assistant/AssistantVehicleQuery.h"
#include "protocol/VehicleProtocol.h"
#include <limits>

class VehicleStateTests:public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){qputenv("CAR_TERMINAL_NO_SETTINGS","1");}
    void assistantQueriesActualReadOnlyState() {
        VehicleState vehicle;
        vehicle.setDriverTemperature(23.5);
        vehicle.setPassengerTemperature(21.5);
        vehicle.beginWindowDrag(0);
        vehicle.previewWindow(0, 37);
        vehicle.endWindowDrag(0);
        vehicle.setWindow(0, 100);
        QVERIFY(assistantVehicleAnswer(vehicle, QStringLiteral("现在还有多少电")).contains(QStringLiteral("82%")));
        QVERIFY(assistantVehicleAnswer(vehicle, QStringLiteral("主驾现在多少度")).contains(QStringLiteral("23.5")));
        QVERIFY(assistantVehicleAnswer(vehicle, QStringLiteral("副驾温度多少")).contains(QStringLiteral("21.5")));
        QVERIFY(assistantVehicleAnswer(vehicle, QStringLiteral("左前车窗开了多少")).contains(QStringLiteral("37%")));
        QVERIFY(assistantVehicleAnswer(vehicle, QStringLiteral("别关空调")).isEmpty());
        QVERIFY(assistantVehicleAnswer(vehicle, QStringLiteral("关闭左前车窗")).isEmpty());
        QCOMPARE(vehicle.windowTargets().at(0).toInt(), 100);
        vehicle.setSourceMode("external");
        QVERIFY(assistantVehicleAnswer(vehicle, QStringLiteral("查询电量")).contains(QStringLiteral("不可用")));
        QVERIFY(assistantVehicleAnswer(vehicle, QStringLiteral("全车车窗状态")).contains(QStringLiteral("不可用")));
    }
    void assistantWritesThroughLocalVehicle() {
        VehicleState vehicle;
        AssistantVehicle assistant(vehicle);
        QString message;
        bool final = false;
        auto reply = [&](const QString &text, bool done) { message = text; final = done; };
        vehicle.beginWindowDrag(0);
        vehicle.previewWindow(0, 60);
        vehicle.endWindowDrag(0);
        QVERIFY(!assistant.submit(QStringLiteral("别关左前窗"), reply));
        QVERIFY(!assistant.submit(QStringLiteral("关左前窗，再开右前窗"), reply));
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 60.0);
        QVERIFY(assistant.submit(QStringLiteral("关左前窗"), reply));
        QCOMPARE(message, QStringLiteral("正在关闭左前车窗…"));
        QVERIFY(!final);
        QVERIFY(vehicle.windowBusy().at(0).toBool());
        QTRY_VERIFY_WITH_TIMEOUT(final, 2500);
        QCOMPARE(message, QStringLiteral("左前车窗已关闭"));
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 0.0);

        vehicle.setPassengerTemperature(20);
        QVERIFY(assistant.submit(QStringLiteral("主驾调到 23 度"), reply));
        QCOMPARE(vehicle.driverTemperature(), 23.0);
        QCOMPARE(vehicle.passengerTemperature(), 20.0);
        QCOMPARE(message, QStringLiteral("主驾空调已设为 23 度"));
        QVERIFY(!assistant.submit(QStringLiteral("主驾别调到二十四度"), reply));
        QVERIFY(!assistant.submit(QStringLiteral("主价调到二十四度"), reply));
        QVERIFY(!assistant.submit(QStringLiteral("别把主架调到二十四度"), reply, true));
        QVERIFY(assistant.submit(QStringLiteral("主驾调到三十五度"), reply));
        QCOMPARE(vehicle.driverTemperature(), 23.0);
        QVERIFY(message.contains(QStringLiteral("范围")));
        vehicle.setClimateSync(true);
        QVERIFY(assistant.submit(QStringLiteral("主驾驶空调调到二十四度"), reply));
        QCOMPARE(vehicle.driverTemperature(), 24.0);
        QCOMPARE(vehicle.passengerTemperature(), 24.0);
        QVERIFY(message.contains(QStringLiteral("已同步")));

        vehicle.beginWindowDrag(0);
        vehicle.previewWindow(0, 60);
        vehicle.endWindowDrag(0);
        QVERIFY(assistant.submit(QStringLiteral("关左前窗"), reply));
        vehicle.setWindow(0, 100);
        QVERIFY(final);
        QVERIFY(message.contains(QStringLiteral("替代")));
    }
    void assistantControlsAllWindowsAndClimate() {
        VehicleState vehicle;
        AssistantVehicle assistant(vehicle);
        QString message;
        bool final = false;
        auto reply = [&](const QString &text, bool done) { message = text; final = done; };
        for (int i = 0; i < 4; ++i) {
            vehicle.beginWindowDrag(i);
            vehicle.previewWindow(i, 10 * (i + 1));
            vehicle.endWindowDrag(i);
        }
        QVERIFY(assistant.submit(QStringLiteral("右后窗开到百分之六十"), reply));
        QVERIFY(!final);
        QTRY_VERIFY_WITH_TIMEOUT(final, 2500);
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 60.0);
        QCOMPARE(vehicle.windowActual().at(2).toDouble(), 30.0);
        QVERIFY(assistant.submit(QStringLiteral("左后窗调到101%"), reply));
        QVERIFY(message.contains(QStringLiteral("范围")));
        QCOMPARE(vehicle.windowActual().at(2).toDouble(), 30.0);
        QVERIFY(!assistant.submit(QStringLiteral("别关全部车窗"), reply));
        QVERIFY(assistant.submit(QStringLiteral("关闭全部车窗"), reply));
        QVERIFY(!final);
        QTRY_VERIFY_WITH_TIMEOUT(final, 2500);
        QCOMPARE(message, QStringLiteral("全部车窗已关闭"));
        for (const auto &opening : vehicle.windowActual())
            QCOMPARE(opening.toDouble(), 0.0);

        QVERIFY(assistant.submit(QStringLiteral("副驾驶温度调到 24.5 度"), reply));
        QCOMPARE(vehicle.passengerTemperature(), 24.5);
        QCOMPARE(vehicle.driverTemperature(), 22.0);
        vehicle.setClimateSync(true);
        QVERIFY(assistant.submit(QStringLiteral("副驾调到二十三度"), reply));
        QCOMPARE(vehicle.driverTemperature(), 23.0);
        QCOMPARE(vehicle.passengerTemperature(), 23.0);
        QVERIFY(assistant.submit(QStringLiteral("空调风量调到四档"), reply));
        QCOMPARE(vehicle.fan(), 4);
        QVERIFY(!assistant.submit(QStringLiteral("风量掉到三档"), reply, true));
        QCOMPARE(vehicle.fan(), 4);
        QVERIFY(assistant.submit(QStringLiteral("风量调到八挡"), reply));
        QVERIFY(message.contains(QStringLiteral("范围")));
        QCOMPARE(vehicle.fan(), 4);
        vehicle.setAcOn(true);
        QVERIFY(!assistant.submit(QStringLiteral("别关掉空调制冷"), reply, true));
        QVERIFY(vehicle.acOn());
        QVERIFY(assistant.submit(QStringLiteral("关闭空调制冷"), reply));
        QVERIFY(!vehicle.acOn());
        QVERIFY(assistant.submit(QStringLiteral("打开A/C"), reply));
        QVERIFY(vehicle.acOn());
    }
    void assistantHandlesSimulatorReceipts() {
        const QString executable = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_TEST_SIMULATOR");
        if (executable.isEmpty())
            QSKIP("Supply the simulator executable to test actual IPC and fault injection");
        const QString server = "assistant-test-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QProcess simulator;
        simulator.setProcessChannelMode(QProcess::ForwardedChannels);
        simulator.start(executable, {"--headless", "--server", server});
        QVERIFY(simulator.waitForStarted());
        VehicleState vehicle;
        AssistantVehicle assistant(vehicle);
        vehicle.connection()->setServerName(server);
        vehicle.setExternalMode(true);
        QTRY_VERIFY_WITH_TIMEOUT(vehicle.externalReady(), 5000);

        QLocalSocket operatorSocket;
        QByteArray buffer;
        QString lastControl;
        connect(&operatorSocket, &QLocalSocket::readyRead, this, [&] {
            buffer += operatorSocket.readAll();
            QList<QJsonObject> messages;
            QString error;
            QVERIFY2(VehicleProtocol::decode(buffer, messages, error), qPrintable(error));
            for (const auto &message : messages) {
                if (message["type"] == "controlResult" && message["ok"].toBool())
                    lastControl = message["id"].toString();
            }
        });
        operatorSocket.connectToServer(server);
        QTRY_COMPARE(operatorSocket.state(), QLocalSocket::ConnectedState);
        operatorSocket.write(VehicleProtocol::encode({{"type","hello"},{"role","operator"}}));
        auto control = [&](const QString &action, const QJsonObject &args) {
            const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            operatorSocket.write(VehicleProtocol::encode({{"type","control"},{"id",id},{"action",action},{"args",args}}));
            QTRY_COMPARE_WITH_TIMEOUT(lastControl, id, 2000);
        };
        QString message;
        bool final = false;
        auto reply = [&](const QString &text, bool done) { message = text; final = done; qInfo() << text; };
        const auto close = QStringLiteral("关左前窗");
        control("window", {{"index",0},{"value",75}});
        QTRY_COMPARE(vehicle.windowActual().at(0).toDouble(), 75.0);
        QVERIFY(assistant.submit(close, reply));
        QTRY_COMPARE(message, QStringLiteral("正在关闭左前车窗…"));
        QVERIFY(!final);
        QVERIFY(vehicle.windowActual().at(0).toDouble() > 0);
        QTRY_VERIFY_WITH_TIMEOUT(final, 3500);
        QCOMPARE(message, QStringLiteral("左前车窗已关闭"));
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 0.0);
        control("window", {{"index",1},{"value",10}});
        control("window", {{"index",3},{"value",85}});
        QTRY_COMPARE(vehicle.windowActual().at(3).toDouble(), 85.0);
        QVERIFY(assistant.submit(QStringLiteral("关闭全部车窗"), reply));
        QTRY_COMPARE(message, QStringLiteral("正在关闭全部车窗…"));
        QTRY_COMPARE(vehicle.windowActual().at(1).toDouble(), 0.0);
        QVERIFY(vehicle.windowActual().at(3).toDouble() > 0);
        QVERIFY(!final);
        QTRY_VERIFY_WITH_TIMEOUT(final, 3500);
        QCOMPARE(message, QStringLiteral("全部车窗已关闭"));
        for (const auto &opening : vehicle.windowActual())
            QCOMPARE(opening.toDouble(), 0.0);
        QVERIFY(assistant.submit(QStringLiteral("右前窗开到35%"), reply));
        QTRY_VERIFY_WITH_TIMEOUT(final, 3500);
        QCOMPARE(message, QStringLiteral("右前车窗开度已调至 35%"));
        QCOMPARE(vehicle.windowActual().at(1).toDouble(), 35.0);

        control("window", {{"index",0},{"value",50}});
        QTRY_COMPARE(vehicle.windowActual().at(0).toDouble(), 50.0);
        control("faults", {{"reject",true}});
        QVERIFY(assistant.submit(close, reply));
        QTRY_VERIFY(final);
        QVERIFY(message.contains(QStringLiteral("被拒绝")));
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 50.0);
        control("faults", {{"reject",false},{"drop",true}});
        QVERIFY(assistant.submit(close, reply));
        QTRY_VERIFY_WITH_TIMEOUT(final, 3500);
        QVERIFY(message.contains(QStringLiteral("超时")));
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 50.0);
        control("faults", {{"drop",false},{"delayMs",1000}});
        QVERIFY(assistant.submit(close, reply));
        control("disconnect", {});
        QTRY_VERIFY(final);
        QVERIFY(message.contains(QStringLiteral("断开")));
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 50.0);
        simulator.kill();
        QVERIFY(simulator.waitForFinished());
    }
    void continuousWindowsStopPreviewAndBatch() {
        VehicleState v;v.setWindow(0,100);QCOMPARE(v.windowTargets()[0].toInt(),100);QCOMPARE(v.windows()[0].toInt(),0);
        QTest::qWait(250);const double halfway=v.windowActual()[0].toDouble();QVERIFY(halfway>5 && halfway<100);
        v.stopWindow(0);const auto stopped=v.windows()[0];QTest::qWait(180);QCOMPARE(v.windows()[0],stopped);QVERIFY(!v.windowBusy()[0].toBool());
        v.beginWindowDrag(1);v.previewWindow(1,35);QCOMPARE(v.windowActual()[1].toDouble(),35.0);v.endWindowDrag(1);
        QCOMPARE(v.windowTargets()[1].toInt(),35);QVERIFY(!v.windowBusy()[1].toBool());
        v.ventilateWindows();QTRY_VERIFY_WITH_TIMEOUT(!v.busy(),2500);for(auto x:v.windows())QCOMPARE(x.toInt(),15);
        v.closeWindows();QTRY_VERIFY_WITH_TIMEOUT(!v.busy(),2500);for(auto x:v.windows())QCOMPARE(x.toInt(),0);
        QSignalSpy messages(&v,&VehicleState::toast);v.closeWindows();QTest::qWait(60);QCOMPARE(messages.size(),0);
        v.setWindow(0,100);v.reset();QTest::qWait(150);QCOMPARE(v.windows()[0].toInt(),0);QVERIFY(!v.busy());
    }
    void routeAndDrivingAreIndependent() {
        VehicleState v;QVERIFY(v.startNavigation(QStringLiteral("回家")));QVERIFY(v.parked());QCOMPARE(v.speed(),0);QCOMPARE(v.remainingMinutes(),28);
        v.advanceRoute(600);QCOMPARE(v.routeProgress(),0.0);
        v.setDoor(0,true);v.startDriving();QVERIFY(v.parked());v.closeDoors();QTRY_VERIFY_WITH_TIMEOUT(!v.busy(),2000);
        v.startDriving();QCOMPARE(v.gear(),QString("D"));QCOMPARE(v.speed(),40);v.advanceSimulation(300);QVERIFY(v.routeProgress()>0);
        const auto progress=v.routeProgress();v.parkVehicle();QVERIFY(v.navigationActive());v.advanceSimulation(300);QCOMPARE(v.routeProgress(),progress);
        v.startDriving();v.endNavigation();QCOMPARE(v.gear(),QString("D"));QCOMPARE(v.speed(),40);
        v.parkVehicle();v.startNavigation(QStringLiteral("公司"));QCOMPARE(v.remainingMinutes(),18);QCOMPARE(v.remainingKm(),9.6);
        QCOMPARE(v.routePreview(QStringLiteral("回家"))["minutes"].toInt(),28);
    }
    void freeDrivingTripReverseAndRestore() {
        VehicleState v;v.startDriving();QVERIFY(!v.navigationActive());v.advanceSimulation(60);QVERIFY(std::abs(v.tripKm()-40.0/60)<0.0001);
        const auto saved=v.snapshot();v.setSpeed(80);v.setGear("R");QCOMPARE(v.speed(),15);v.advanceSimulation(60);QVERIFY(v.tripKm()>saved["tripKm"].toDouble());
        v.restoreSnapshot(saved);QCOMPARE(v.tripKm(),saved["tripKm"].toDouble());v.parkVehicle();const double trip=v.tripKm();v.advanceSimulation(60);QCOMPARE(v.tripKm(),trip);
        v.resetRuntime();QCOMPARE(v.tripKm(),0.0);
    }
    void chargeConnectionTargetAndInterlocks() {
        VehicleState v;v.startCharging();QVERIFY(!v.charging());v.togglePort();v.togglePlug();QVERIFY(v.plugged());
        v.togglePort();QVERIFY(v.portOpen());v.startDriving();QVERIFY(v.parked());
        v.startCharging();QVERIFY(v.charging());const auto before=v.batteryPercent();v.advanceSimulation(120);QVERIFY(v.batteryPercent()>before);
        v.setGear("N");QCOMPARE(v.gear(),QString("P"));QVERIFY(v.charging());
        v.togglePlug();QVERIFY(v.plugged());v.setChargeTarget(95);v.advanceSimulation(3600);QCOMPARE(v.batteryPercent(),95.0);QVERIFY(!v.charging());
        v.togglePlug();v.togglePort();QVERIFY(!v.portOpen());v.startDriving();QCOMPARE(v.gear(),QString("D"));
    }
    void climateSyncAndRestoration() {
        VehicleState v;v.setDriverTemperature(40);QCOMPARE(v.driverTemperature(),30.0);v.setPassengerTemperature(-1);QCOMPARE(v.passengerTemperature(),16.0);
        v.setDriverTemperature(std::numeric_limits<double>::quiet_NaN());QCOMPARE(v.driverTemperature(),30.0);
        v.setClimateSync(true);v.setPassengerTemperature(20.5);QCOMPARE(v.driverTemperature(),20.5);
        v.setFan(4);v.setAcOn(false);v.setDefrost(true);QVERIFY(v.acOn());QCOMPARE(v.fan(),5);v.setDefrost(false);QCOMPARE(v.fan(),4);QVERIFY(!v.acOn());
        v.setScene("rest");QCOMPARE(v.driverTemperature(),23.0);v.setScene("comfort");QCOMPARE(v.driverTemperature(),20.5);QCOMPARE(v.fan(),4);
        v.setHeatLevel(9);QCOMPARE(v.heatLevel(),3);
        v.setAirDirection("feet");v.setDefrost(true);v.setScene("camp");v.setHeatLevel(0);v.setAirDirection("face");v.setClimateSync(false);v.setScene("comfort");
        QVERIFY(v.defrost());QVERIFY(v.climateSync());QCOMPARE(v.airDirection(),QString("feet"));QCOMPARE(v.heatLevel(),3);v.setDefrost(false);QCOMPARE(v.fan(),4);QVERIFY(!v.acOn());
    }
    void notificationsAndSnapshotCancellation() {
        VehicleState v;const auto initial=v.snapshot();v.setDoor(1,true);v.toggleHatch();v.setWindow(3,50);QTRY_VERIFY_WITH_TIMEOUT(!v.busy(),2500);
        QCOMPARE(v.warningCount(),2);QVERIFY(!v.notifications().isEmpty());
        v.setWindow(0,100);v.restoreSnapshot(initial);QSignalSpy messages(&v,&VehicleState::toast);QTest::qWait(950);
        QVERIFY(!v.busy());QVERIFY(v.notifications().isEmpty());QVERIFY(v.parked());QCOMPARE(messages.size(),0);
    }
    void preferenceRoundTripDoesNotPersistOpenParts() {
        QTemporaryDir dir;QVERIFY(dir.isValid());qunsetenv("CAR_TERMINAL_NO_SETTINGS");qputenv("CAR_TERMINAL_SETTINGS_FILE",(dir.path()+"/prefs.ini").toUtf8());
        {
            VehicleState v;v.setDriverTemperature(20.5);v.setThemeMode("night");v.setAirDirection("feet");v.setAutoFocus(false);v.setWindow(0,100);QTest::qWait(450);
            const auto saved=v.snapshot();v.setDemoActive(true);v.setDriverTemperature(30);QTest::qWait(450);
            QSettings stored(dir.path()+"/prefs.ini",QSettings::IniFormat);QCOMPARE(stored.value("driver").toDouble(),20.5);
            v.restoreSnapshot(saved);v.setDemoActive(false);
        }
        {
            VehicleState v;QCOMPARE(v.driverTemperature(),20.5);QCOMPARE(v.airDirection(),QString("feet"));QVERIFY(v.night());QVERIFY(!v.autoFocus());QCOMPARE(v.windows()[0].toInt(),0);QVERIFY(v.parked());
        }
        qunsetenv("CAR_TERMINAL_SETTINGS_FILE");qputenv("CAR_TERMINAL_NO_SETTINGS","1");
    }
};
QTEST_GUILESS_MAIN(VehicleStateTests)
#include "VehicleStateTests.moc"

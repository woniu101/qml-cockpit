#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include "VehicleState.h"
#include <limits>

class VehicleStateTests:public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){qputenv("CAR_TERMINAL_NO_SETTINGS","1");}
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
    void mediaRowAndRepeatSemantics() {
        VehicleState v;v.selectTrack(0);QVERIFY(v.playing());v.selectTrack(0);QVERIFY(!v.playing());v.selectTrack(1);QVERIFY(v.playing());QCOMPARE(v.track(),1);
        v.updateDuration(1000);QSignalSpy seeks(&v,&VehicleState::seekRequested);v.seek(9999);QCOMPARE(seeks.last()[0].toLongLong(),1000LL);
        v.setPlayMode("one");v.finishTrack();QCOMPARE(v.track(),1);QCOMPARE(seeks.last()[0].toLongLong(),0LL);
    }
    void preferenceRoundTripDoesNotPersistOpenParts() {
        QTemporaryDir dir;QVERIFY(dir.isValid());qunsetenv("CAR_TERMINAL_NO_SETTINGS");qputenv("CAR_TERMINAL_SETTINGS_FILE",(dir.path()+"/prefs.ini").toUtf8());
        {
            VehicleState v;v.setDriverTemperature(20.5);v.setVolume(63);v.setThemeMode("night");v.setAirDirection("feet");v.setAutoFocus(false);v.setWindow(0,100);QTest::qWait(450);
            const auto saved=v.snapshot();v.setDemoActive(true);v.setDriverTemperature(30);v.setVolume(5);QTest::qWait(450);
            QSettings stored(dir.path()+"/prefs.ini",QSettings::IniFormat);QCOMPARE(stored.value("driver").toDouble(),20.5);QCOMPARE(stored.value("volume").toInt(),63);
            v.restoreSnapshot(saved);v.setDemoActive(false);
        }
        {
            VehicleState v;QCOMPARE(v.driverTemperature(),20.5);QCOMPARE(v.volume(),63);QCOMPARE(v.airDirection(),QString("feet"));QVERIFY(v.night());QVERIFY(!v.autoFocus());QCOMPARE(v.windows()[0].toInt(),0);QVERIFY(v.parked());
        }
        qunsetenv("CAR_TERMINAL_SETTINGS_FILE");qputenv("CAR_TERMINAL_NO_SETTINGS","1");
    }
};
QTEST_GUILESS_MAIN(VehicleStateTests)
#include "VehicleStateTests.moc"

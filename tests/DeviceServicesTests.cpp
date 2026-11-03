#include <QtTest>
#include "AudioDevices.h"
#include "BluetoothStatus.h"
#include <QMediaPlayer>
#include <QTemporaryFile>
#include <QDataStream>

class DeviceServicesTests:public QObject {
    Q_OBJECT
private slots:
    void initTestCase(){qputenv("CAR_TERMINAL_NO_SETTINGS","1");}
    void defaultAndExplicitRoutes(){
        auto r=resolveAudioRoute({"a","b"},"a",{});QCOMPARE(r.id,QString("a"));QVERIFY(r.followDefault);QVERIFY(!r.fellBack);
        r=resolveAudioRoute({"a","b"},"b","a");QCOMPARE(r.id,QString("a"));QVERIFY(!r.followDefault);
        r=resolveAudioRoute({"a","b"},"b",{});QCOMPARE(r.id,QString("b"));QVERIFY(r.followDefault);
        r=resolveAudioRoute({"a"},"a","b");QCOMPARE(r.id,QString("a"));QVERIFY(r.fellBack);QVERIFY(r.followDefault);
        r=resolveAudioRoute({},"a","b");QVERIFY(r.id.isEmpty());QVERIFY(r.fellBack);
        r=resolveAudioRoute({"a"},"missing",{});QCOMPARE(r.id,QString("a"));
    }
    void actualOutputsAndRemoval(){
        const auto devices=QMediaDevices::audioOutputs();if(devices.isEmpty())QSKIP("No physical outputs; routing policy still covered");
        QAudioOutput output;output.setVolume(0);AudioDevices service(&output,nullptr,false);
        service.applyDevices(devices,devices.first());QVERIFY(service.available());QVERIFY(service.followDefault());QVERIFY(service.routeMatches());
        const auto selected=QString::fromLatin1(devices.last().id().toHex());QVERIFY(service.selectOutput(selected));QVERIFY(!service.followDefault());QVERIFY(service.routeMatches());
        service.applyDevices(devices,devices.first());QCOMPARE(output.device().id(),devices.last().id());
        QVERIFY(!service.selectOutput("unavailable"));QCOMPARE(output.device().id(),devices.last().id());
        QSignalSpy unavailable(&service,&AudioDevices::outputUnavailable);service.applyDevices({},{});QVERIFY(!service.available());QVERIFY(service.followDefault());QCOMPARE(unavailable.count(),1);
        service.applyDevices(devices,devices.first());QVERIFY(service.followDefault());QCOMPARE(output.device().id(),devices.first().id());QCOMPARE(output.volume(),0.0f);
    }
    void bluetoothStatesAndPairingAreDistinct(){
        BluetoothStatus service(nullptr,false);
        for(const auto &state:QStringList{"absent","off","disabled","unknown","denied","error","unsupported"}){
            BluetoothSnapshot sample;sample.state=state;service.applySnapshot(sample);QCOMPARE(service.status()["state"].toString(),state);QCOMPARE(service.status()["connectedCount"].toInt(),0);
        }
        BluetoothSnapshot sample;sample.state="on";sample.devices={QVariantMap{{"name","paired phone"},{"connectionKnown",true},{"connected",false}},QVariantMap{{"name","keyboard"},{"connectionKnown",true},{"connected",true}},QVariantMap{{"name","unknown"},{"connectionKnown",false},{"connected",true}}};
        service.applySnapshot(sample);QCOMPARE(service.status()["connectedCount"].toInt(),1);QVERIFY(!service.status()["text"].toString().contains(QStringLiteral("手机")));QVERIFY(!service.busy());
    }
    void mutedPlaybackAcrossActualOutputs(){
        const auto devices=QMediaDevices::audioOutputs();if(devices.isEmpty())QSKIP("No output hardware for actual playback routes");
        QTemporaryFile wave;QVERIFY(wave.open());QDataStream stream(&wave);stream.setByteOrder(QDataStream::LittleEndian);
        const QByteArray silence(22050*2*15,'\0');
        stream.writeRawData("RIFF",4);stream<<quint32(36+silence.size());stream.writeRawData("WAVEfmt ",8);stream<<quint32(16)<<quint16(1)<<quint16(1)<<quint32(22050)<<quint32(44100)<<quint16(2)<<quint16(16);stream.writeRawData("data",4);stream<<quint32(silence.size());stream.writeRawData(silence.constData(),silence.size());wave.flush();
        QAudioOutput output;output.setVolume(0);AudioDevices service(&output,nullptr,false);service.applyDevices(devices,devices.first());
        QMediaPlayer player;player.setAudioOutput(&output);player.setSource(QUrl::fromLocalFile(wave.fileName()));player.play();
        QTRY_COMPARE_WITH_TIMEOUT(player.playbackState(),QMediaPlayer::PlayingState,6000);QTRY_VERIFY_WITH_TIMEOUT(player.duration()>1000,6000);
        for(const auto &device:devices){
            const auto position=player.position();QVERIFY(service.selectOutput(QString::fromLatin1(device.id().toHex())));
            QTest::qWait(200);QVERIFY(service.routeMatches());QCOMPARE(player.playbackState(),QMediaPlayer::PlayingState);QVERIFY(player.position()>=position);QCOMPARE(output.volume(),0.0f);
        }
        connect(&service,&AudioDevices::outputUnavailable,&player,&QMediaPlayer::pause);service.applyDevices({},{});QTRY_COMPARE(player.playbackState(),QMediaPlayer::PausedState);
        service.applyDevices(devices,devices.first());QTest::qWait(100);QCOMPARE(player.playbackState(),QMediaPlayer::PausedState);
        qInfo()<<"Actual muted playback routes checked:"<<devices.size()<<"; returned devices did not resume playback";
    }
};
QTEST_GUILESS_MAIN(DeviceServicesTests)
#include "DeviceServicesTests.moc"

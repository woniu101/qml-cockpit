#include "DeviceSmoke.h"
#include "AudioDevices.h"
#include "BluetoothStatus.h"
#include "media/MediaState.h"
#include "MusicFixture.h"
#include <QTemporaryDir>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QMediaPlayer>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTimer>
#include <QtTest>
#include <functional>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace {
class UiReceiverFixture:public ReceiverBackend {
    bool m_open=false;
public:
    ReceiverScan scan(const Cancelled&) override{return {true,{QVariantMap{{"token","fixture"},{"name",QStringLiteral("测试手机 · 音频连接样本")}}},{}};}
    ReceiverOpen open(const QString&,const Cancelled &cancel) override{
        for(int i=0;i<20&&!cancel();++i)std::this_thread::sleep_for(std::chrono::milliseconds(30));
        m_open=!cancel();return {m_open,{}};
    }
    bool connected() override{return m_open;}
    void close() noexcept override{m_open=false;}
};
}
BluetoothAudio::Factory deviceReceiverFixture(){return []{return std::make_unique<UiReceiverFixture>();};}

void runDeviceSmoke(QGuiApplication &app,QQuickWindow &window,MediaState &vehicle,QMediaPlayer &player,AudioDevices &outputs,BluetoothStatus &bluetooth,BluetoothAudio &receiver,QStringList &warnings){
    const auto realDevices=QMediaDevices::audioOutputs();const auto realDefault=QMediaDevices::defaultAudioOutput();
    outputs.setSamplingEnabled(false);outputs.applyDevices(realDevices,realDefault);bluetooth.setSamplingEnabled(false);window.setProperty("testMode",true);window.setProperty("clockText","16:28");
    QTimer::singleShot(1000,&app,[&app,&window,&vehicle,&player,&outputs,&bluetooth,&receiver,&warnings,realDevices,realDefault]{
        const QString directory=qEnvironmentVariable("CAR_TERMINAL_DEVICE_OUTPUT","artifacts/devices/ui");QDir().mkpath(directory);window.raise();window.requestActivate();
        auto require=[](bool ok,const char *reason){if(!ok)throw std::runtime_error(reason);};
        std::function<QQuickItem*(QQuickItem*,const QString&)> find;find=[&](QQuickItem *p,const QString &name)->QQuickItem*{if(p->objectName()==name&&p->isVisible())return p;for(auto *c:p->childItems())if(auto *hit=find(c,name))return hit;return nullptr;};
        auto item=[&](const char *name){auto *hit=find(window.contentItem(),name);require(hit,name);return hit;};
        auto click=[&](const char *name){auto *hit=item(name);require(hit->isEnabled(),name);QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,hit->mapToScene(QPointF(hit->width()/2,hit->height()/2)).toPoint());QTest::qWait(100);};
        auto wait=[&](std::function<bool()> check,const char *reason){for(int i=0;i<300&&!check();++i)QTest::qWait(20);require(check(),reason);};
        auto capture=[&](const QString &name){window.setProperty("toastMessage","");window.setProperty("feedbackQueue",QVariantList{});QTest::qWait(250);require(window.grabWindow().save(directory+"/"+name+".png"),"device screenshot");};
        int actualRoutes=0;
        try{
            QTemporaryDir fixture;
            const auto audio = fixture.filePath("device-test.wav");
            require(writeMusicFixture(audio), "device audio fixture");
            vehicle.importTracks({QUrl::fromLocalFile(audio)});
            for(const auto &state:QStringList{"absent","off","disabled","unknown","denied","error","on"}){
                BluetoothSnapshot sample;sample.state=state;
                if(state=="on"){
                    sample.adapters={QVariantMap{{"name",QStringLiteral("测试适配器")},{"state","on"}}};
                    sample.devices={QVariantMap{{"name",QStringLiteral("测试键盘")},{"kind",QStringLiteral("低功耗蓝牙")},{"connectionKnown",true},{"connected",true}},QVariantMap{{"name",QStringLiteral("已配对手机")},{"kind",QStringLiteral("经典蓝牙")},{"connectionKnown",true},{"connected",false}}};
                }
                bluetooth.applySnapshot(sample);click("bluetoothStatusButton");require(item("bluetoothStateText")->property("text")==bluetooth.status()["text"],"Bluetooth status is real state, not an audio connection");capture("bluetooth-"+state);click("closePopup");
            }
            click("tab3");click("musicAudioOutputs");require(window.property("popup")=="audio","music audio output entry");capture("01-audio-outputs");
            if(!realDevices.isEmpty()){
                click("audioOutput0");require(!outputs.followDefault()&&outputs.routeMatches(),"UI selects an actual QAudioOutput device");++actualRoutes;capture("02-explicit-output");click("closePopup");click("fullMusicPlay");wait([&]{return player.playbackState()==QMediaPlayer::PlayingState&&player.duration()>1000;},"real muted local player running");
                const auto position=player.position();const auto volume=vehicle.volume();
                if(realDevices.size()>1){require(outputs.selectOutput(QString::fromLatin1(realDevices.last().id().toHex())),"switch to another real output");++actualRoutes;}
                QTest::qWait(250);require(outputs.routeMatches()&&vehicle.playing()&&player.position()>=position,"switching output preserves playback and position");require(vehicle.volume()==volume,"output switch preserves local volume");
                outputs.applyDevices(realDevices,realDevices.last());require(!outputs.followDefault(),"explicit route ignores default changes");outputs.selectOutput("");require(outputs.followDefault()&&outputs.routeMatches(),"follow changed default");
                outputs.applyDevices({},{});require(!vehicle.playing()&&player.playbackState()!=QMediaPlayer::PlayingState,"no output pauses local playback");require(!item("fullMusicPlay")->isEnabled(),"unavailable audio cannot claim playback");
            }else click("closePopup");
            click("audioOutputShortcut");capture("03-no-output");outputs.applyDevices(realDevices,realDefault);require(!vehicle.playing(),"returning outputs do not auto-play music");capture("04-output-returned");click("audioFollowDefault");
            window.resize(1120,630);QTest::qWait(250);capture("05-minimum-audio");click("closePopup");click("bluetoothStatusButton");capture("06-minimum-bluetooth");click("closePopup");
            if(!realDevices.isEmpty()){
                click("fullMusicPlay");wait([&]{return player.playbackState()==QMediaPlayer::PlayingState;},"muted local music before phone connection");
                click("musicPhoneAudio");click("musicPhoneDetails");wait([&]{return !receiver.scanning();},"receiver fixture enumeration");click("phoneAudioConnect0");
                require(!vehicle.playing()&&player.playbackState()!=QMediaPlayer::PlayingState,"phone connection pauses actual local player");
                capture("07-phone-connecting");wait([&]{return receiver.phase()=="connected";},"receiver fixture connected");capture("08-phone-connected");
                click("closePopup");require(!find(window.contentItem(),"fullMusicPlay"),"local playback controls hidden during phone reception");
                vehicle.setPlaying(true);QTest::qWait(100);require(!vehicle.playing(),"central playback guard rejects local play while receiving");capture("09-phone-music");
                click("tab0");capture("10-phone-home");click("homePhoneAudio");click("phoneAudioDisconnect");wait([&]{return !receiver.active();},"manual disconnect");
                require(!vehicle.playing(),"disconnect does not auto-resume local music");click("closePopup");click("tab3");click("musicPhoneAudio");click("musicPhoneDetails");wait([&]{return !receiver.scanning();},"enumeration before reconnect");click("phoneAudioConnect0");wait([&]{return receiver.phase()=="connected";},"reconnect fixture");
                outputs.applyDevices({},{});wait([&]{return !receiver.active();},"output removal disconnects phone reception");outputs.applyDevices(realDevices,realDefault);require(!receiver.active()&&!vehicle.playing(),"output return does not reconnect or auto-play");
                click("closePopup");
            }
            require(warnings.isEmpty(),qPrintable(warnings.join('\n')));
            QFile report(directory+"/verification.json");require(report.open(QIODevice::WriteOnly),"device evidence");report.write(QJsonDocument(QJsonObject{{"actualOutputCount",realDevices.size()},{"actualRoutesChecked",actualRoutes},{"routeMatches",outputs.routeMatches()},{"bluetoothStates","fixtures"},{"phoneReceiver",realDevices.isEmpty()?"skipped; no audio output":"fixture; real local-player pause/guard and output-removal integration"},{"hotplugTopology","fixtures using actual QAudioDevice objects"},{"automaticSound","muted"}}).toJson());
            qInfo()<<"PASS devices UI: Bluetooth states, pairing distinction, actual Qt audio routes, output fallback, no auto-play and minimum window";app.exit(0);
        }catch(const std::exception &error){window.grabWindow().save(directory+"/failure.png");qCritical()<<"DEVICES UI FAIL:"<<error.what();app.exit(2);}
    });
}

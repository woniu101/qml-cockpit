#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFile>
#include "VehicleState.h"
#include "IconProvider.h"
#include "UiSmoke.h"
#include "LinkSmoke.h"
#include "SystemSmoke.h"
#include "AudioDevices.h"
#include "BluetoothStatus.h"
#include "BluetoothAudio.h"
#include "DeviceSmoke.h"
#include "MusicLibrary.h"
#include "MusicSmoke.h"
#include "MusicSource.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QDateTime>
#include <memory>

int main(int argc,char *argv[]) {
    bool smoke=false,native=false,linkSmoke=false,systemSmoke=false,systemProbe=false,deviceSmoke=false,deviceProbe=false,deviceQmlCheck=false,musicSmoke=false;
    for(int i=1;i<argc;++i){const QByteArray argument(argv[i]);if(argument=="--smoke-test")smoke=true;if(argument=="--smoke-native"){smoke=true;native=true;}if(argument=="--link-smoke"){smoke=true;native=true;linkSmoke=true;}if(argument=="--system-smoke"){smoke=true;native=true;systemSmoke=true;}if(argument=="--system-probe")systemProbe=true;}
    for(int i=1;i<argc;++i){const QByteArray argument(argv[i]);if(argument=="--device-smoke"){deviceSmoke=true;smoke=true;native=true;}if(argument=="--device-probe")deviceProbe=true;}
    for(int i=1;i<argc;++i)if(QByteArray(argv[i])=="--device-qml-check")deviceQmlCheck=true;
    for(int i=1;i<argc;++i)if(QByteArray(argv[i])=="--music-smoke"||QByteArray(argv[i])=="--music-resume-check"){musicSmoke=true;smoke=true;native=true;}
    if(deviceQmlCheck)qputenv("QT_QPA_PLATFORM","offscreen");
    if(systemProbe||deviceProbe||deviceQmlCheck){qputenv("CAR_TERMINAL_NO_SETTINGS","1");qputenv("QT_FORCE_STDERR_LOGGING","1");}
    if(smoke){if(!native)qputenv("QSG_RENDER_LOOP","basic");qputenv("QT_FORCE_STDERR_LOGGING","1");qputenv("CAR_TERMINAL_NO_SETTINGS","1");}
    QGuiApplication app(argc,argv);app.setApplicationName("CarTerminal");app.setOrganizationName("CarTerminalDemo");QQuickStyle::setStyle("Basic");
    QAudioOutput audio;AudioDevices outputs(&audio);BluetoothStatus bluetooth(nullptr,!deviceSmoke&&!deviceQmlCheck);
    BluetoothAudio receiver(nullptr,deviceSmoke?deviceReceiverFixture():BluetoothAudio::Factory{},(!smoke||deviceSmoke)&&!deviceQmlCheck);receiver.setOutputAvailable(outputs.available());
    QQmlApplicationEngine engine;QStringList warnings;
    QObject::connect(&engine,&QQmlEngine::warnings,&app,[&](const QList<QQmlError> &errors){for(const auto &error:errors)warnings<<error.toString();});
    engine.addImageProvider("icons",new IconProvider);
    engine.rootContext()->setContextProperty("vehicleAssetsAvailable",QFile::exists(":/qt/qml/CarTerminal/assets/sedan/meshes/body_primary_0_mesh.mesh"));
    auto *state=engine.singletonInstance<VehicleState*>("CarTerminal","Vehicle");if(!state)return 1;
    engine.rootContext()->setContextProperty("VehicleConnection",state->connection());
    engine.rootContext()->setContextProperty("Host",state->systemStatus());
    engine.rootContext()->setContextProperty("AudioOutputs",&outputs);
    engine.rootContext()->setContextProperty("BluetoothState",&bluetooth);
    engine.rootContext()->setContextProperty("PhoneAudio",&receiver);
    std::unique_ptr<QIODevice> sourceDevice;QMediaPlayer player;player.setAudioOutput(&audio);MusicLibrary library(state,&player,nullptr,qEnvironmentVariable("CAR_TERMINAL_MUSIC_LIBRARY_FILE"));qint64 pendingSeek=-1;
    engine.rootContext()->setContextProperty("Music",&library);
    QObject::connect(&library,&MusicLibrary::notice,&app,[&](const QString &message){emit state->toast(message,"info");});
    for(const auto &argument:app.arguments())if(argument.startsWith("--vehicle-server="))state->connection()->setServerName(argument.mid(17));
    if(app.arguments().contains("--external"))state->setExternalMode(true);
    for(const auto &argument:app.arguments())if(argument.startsWith("--source="))state->setSourceMode(argument.mid(9));
    if(systemProbe){
        QEventLoop loop;QTimer::singleShot(600,&loop,&QEventLoop::quit);loop.exec();state->systemStatus()->refresh();
        QDir().mkpath("artifacts/system");QFile output("artifacts/system/live.json");if(!output.open(QIODevice::WriteOnly))return 2;
        output.write(QJsonDocument(QJsonObject{{"capturedAt",QDateTime::currentDateTime().toString(Qt::ISODate)},{"power",QJsonObject::fromVariantMap(state->systemStatus()->power())},{"network",QJsonObject::fromVariantMap(state->systemStatus()->network())},{"source",state->sourceMode()},{"fallback",state->systemFallback()}}).toJson());return 0;
    }
    if(deviceQmlCheck){
        BluetoothSnapshot fixture;fixture.state="on";fixture.adapters={QVariantMap{{"name",QStringLiteral("测试适配器")},{"state","on"}}};fixture.devices={QVariantMap{{"name",QStringLiteral("测试键盘")},{"kind",QStringLiteral("低功耗蓝牙")},{"connected",true},{"connectionKnown",true}}};bluetooth.applySnapshot(fixture);
        QQmlComponent stub(&engine);stub.setData("import QtQml\nQtObject { property string clockText: '16:28'; property string popup: ''; property int page: 0; property bool demoRunning: false; function formatTime(value) { return '00:00' } function openPopup(name) {} function showPage(index) {} function inspectPart(part) {} }",QUrl());
        std::unique_ptr<QObject> shell(stub.create());if(!shell)return 2;QJsonArray checked;qsizetype windowObjects=0;
        for(const auto &name:QStringList{"AudioPanel","BluetoothPanel","MusicPage","MusicManagerPanel","StatusBar","BottomBar","MusicCard"}){
            QQmlComponent component(&engine);component.loadFromModule("CarTerminal",name);QVariantMap properties;
            if(name!="AudioPanel")properties["shell"]=QVariant::fromValue(shell.get());
            std::unique_ptr<QObject> object(component.createWithInitialProperties(properties));
            if(!object){qCritical()<<component.errors();return 2;}QCoreApplication::processEvents();windowObjects=qMax(windowObjects,QGuiApplication::allWindows().size());checked.append(name);
        }
        QDir().mkpath("artifacts/devices");QFile file("artifacts/devices/qml-check.json");if(!file.open(QIODevice::WriteOnly))return 2;
        file.write(QJsonDocument(QJsonObject{{"components",checked},{"platform",QGuiApplication::platformName()},{"qtWindowObjects",windowObjects},{"desktopInputUsed",false},{"warnings",QJsonArray::fromStringList(warnings)}}).toJson());
        if(!warnings.isEmpty()){qCritical()<<warnings;return 2;}qInfo()<<"PASS device QML bindings: offscreen components, no desktop windows or input";return 0;
    }
    if(deviceProbe){
        QEventLoop loop;auto ready=[&]{if(!bluetooth.busy()&&bluetooth.status()["state"]!="pending"&&!receiver.scanning())loop.quit();};
        QObject::connect(&bluetooth,&BluetoothStatus::changed,&loop,ready);QObject::connect(&receiver,&BluetoothAudio::changed,&loop,ready);
        bluetooth.refresh();receiver.refresh();QTimer::singleShot(12000,&loop,&QEventLoop::quit);loop.exec();outputs.refresh();
        QVariantList devices;for(const auto &row:outputs.outputs()){auto value=row.toMap();value.remove("id");devices<<value;}
        QVariantList phones;for(const auto &row:receiver.devices()){auto value=row.toMap();value.remove("token");phones<<value;}
        QDir().mkpath("artifacts/devices");QFile file("artifacts/devices/live.json");if(!file.open(QIODevice::WriteOnly))return 2;
        file.write(QJsonDocument(QJsonObject{{"capturedAt",QDateTime::currentDateTime().toString(Qt::ISODate)},{"bluetooth",QJsonObject::fromVariantMap(bluetooth.status())},{"phoneAudio",QJsonObject{{"supported",receiver.supported()},{"phase",receiver.phase()},{"devices",QJsonArray::fromVariantList(phones)},{"scanError",receiver.scanError()}}},{"audioOutputs",QJsonArray::fromVariantList(devices)},{"activeOutput",outputs.activeName()},{"followDefault",outputs.followDefault()},{"routeMatches",outputs.routeMatches()}}).toJson());return 0;
    }
    auto synchronize=[&]{
        const QUrl source(state->trackUrl());if(player.source()!=source){pendingSeek=state->position()>0?qint64(state->position()):-1;setMusicSource(player,source,sourceDevice);}
        audio.setVolume(smoke?0.0f:state->volume()/100.0f);
        if(state->playing()&&receiver.active()){state->setPlaying(false);return;}
        if(state->playing()&&!outputs.available()){state->setPlaying(false);emit state->toast(QStringLiteral("没有可用音频输出，请连接扬声器或耳机"),"warning");return;}
        if(state->playing())player.play();else player.pause();
    };
    QObject::connect(state,&VehicleState::changed,&app,synchronize);
    QObject::connect(&outputs,&AudioDevices::notice,&app,[&](const QString &message){emit state->toast(message,"info");});
    QObject::connect(&bluetooth,&BluetoothStatus::notice,&app,[&](const QString &message){emit state->toast(message,"warning");});
    QObject::connect(&receiver,&BluetoothAudio::notice,&app,[&](const QString &message){emit state->toast(message,"warning");});
    QString receiverLogPhase;
    QObject::connect(&receiver,&BluetoothAudio::changed,&app,[&]{if(receiver.active())state->setPlaying(false);if(receiverLogPhase!=receiver.phase()){receiverLogPhase=receiver.phase();qInfo().noquote()<<"Phone audio:"<<receiverLogPhase<<receiver.error();}});
    QObject::connect(&receiver,&BluetoothAudio::changed,&bluetooth,[&]{if(receiver.phase()=="connected"||receiver.phase()=="idle")bluetooth.refresh();});
    QObject::connect(&bluetooth,&BluetoothStatus::changed,&receiver,[&]{const auto radio=bluetooth.status()["state"].toString();if(receiver.active()&&(radio=="off"||radio=="absent"||radio=="disabled"))receiver.disconnectDevice();});
    QObject::connect(&outputs,&AudioDevices::changed,&receiver,[&]{receiver.setOutputAvailable(outputs.available());});
    QObject::connect(&outputs,&AudioDevices::outputUnavailable,&app,[&]{const bool playing=state->playing();state->setPlaying(false);emit state->toast(playing?QStringLiteral("音频输出已断开，本地音乐已暂停"):QStringLiteral("未检测到音频输出设备"),"warning");});
    bool seekScheduled=false;
    auto applyPendingSeek=[&]{
        if(pendingSeek<0||seekScheduled||!player.isSeekable()||player.duration()<=0)return;
        if(player.mediaStatus()!=QMediaPlayer::LoadedMedia&&player.mediaStatus()!=QMediaPlayer::BufferedMedia)return;
        seekScheduled=true;const auto source=player.source();
        // FFmpeg emits load signals before finishing its backend setup. Apply
        // the bookmark after that event has returned, without starting playback.
        QTimer::singleShot(0,&app,[&,source]{seekScheduled=false;if(player.source()!=source||pendingSeek<0)return;const auto at=qMin(pendingSeek,qMax(0LL,player.duration()-1000));pendingSeek=-1;player.setPosition(at);});
    };
    QObject::connect(state,&VehicleState::seekRequested,&app,[&](qint64 at){if(player.isSeekable()){pendingSeek=-1;player.setPosition(at);}else pendingSeek=at;});
    QObject::connect(&player,&QMediaPlayer::seekableChanged,&app,[&](bool){applyPendingSeek();});
    QObject::connect(state,&VehicleState::mediaReloadRequested,&app,[&]{setMusicSource(player,QUrl(state->trackUrl()),sourceDevice);synchronize();});
    QObject::connect(&player,&QMediaPlayer::positionChanged,state,&VehicleState::updatePosition);
    QObject::connect(&player,&QMediaPlayer::durationChanged,&app,[&](qint64 value){state->updateDuration(value);applyPendingSeek();});
    QObject::connect(&player,&QMediaPlayer::mediaStatusChanged,&app,[&](QMediaPlayer::MediaStatus status){
        if(status==QMediaPlayer::EndOfMedia){state->finishTrack();return;}
        if(status==QMediaPlayer::LoadingMedia)state->updateMediaStatus("loading");
        if(status==QMediaPlayer::LoadedMedia||status==QMediaPlayer::BufferedMedia){state->updateMediaStatus("ready");applyPendingSeek();}
    });
    QObject::connect(&player,&QMediaPlayer::errorOccurred,&app,[&](QMediaPlayer::Error,const QString &message){state->updateMediaStatus("error",message);emit state->toast(QStringLiteral("音源加载失败，可在音乐页重试"),"warning");});
    synchronize();
    QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(1);},Qt::QueuedConnection);
    engine.loadFromModule("CarTerminal",app.arguments().contains("--legacy-3d")?"Legacy3D":"Main");if(engine.rootObjects().isEmpty())return 1;
    auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if(musicSmoke)runMusicSmoke(app,*window,*state,player,library,warnings);else if(deviceSmoke)runDeviceSmoke(app,*window,*state,player,outputs,bluetooth,receiver,warnings);else if(systemSmoke)runSystemSmoke(app,*window,*state,warnings);else if(linkSmoke)runLinkSmoke(app,*window,*state,warnings);else if(smoke)runUiSmoke(app,*window,*state,player,warnings,native);
    const int result=app.exec();library.save();
    // Destroy bindings while all context services and the Vehicle singleton live.
    for(auto *root:engine.rootObjects())delete root;
    return result;
}

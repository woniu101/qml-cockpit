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
#include "WeatherService.h"
#include "MediaSmoke.h"
#include "assistant/AssistantRuntime.h"
#include "media/MediaState.h"
#include "media/MediaSession.h"
#include "media/RadioService.h"
#include "media/VideoLibrary.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QDateTime>
#include <memory>

int main(int argc,char *argv[]) {
    bool smoke=false,native=false,linkSmoke=false,systemSmoke=false,systemProbe=false,deviceSmoke=false,deviceProbe=false,deviceQmlCheck=false,musicSmoke=false,mediaSmoke=false;
    for(int i=1;i<argc;++i){const QByteArray argument(argv[i]);if(argument=="--smoke-test")smoke=true;if(argument=="--smoke-native"){smoke=true;native=true;}if(argument=="--link-smoke"){smoke=true;native=true;linkSmoke=true;}if(argument=="--system-smoke"){smoke=true;native=true;systemSmoke=true;}if(argument=="--system-probe")systemProbe=true;}
    for(int i=1;i<argc;++i){const QByteArray argument(argv[i]);if(argument=="--device-smoke"){deviceSmoke=true;smoke=true;native=true;}if(argument=="--device-probe")deviceProbe=true;}
    for(int i=1;i<argc;++i)if(QByteArray(argv[i])=="--device-qml-check")deviceQmlCheck=true;
    for(int i=1;i<argc;++i)if(QByteArray(argv[i])=="--music-smoke"||QByteArray(argv[i])=="--music-resume-check"){musicSmoke=true;smoke=true;native=true;}
    for(int i=1;i<argc;++i)if(QByteArray(argv[i])=="--media-smoke"){mediaSmoke=true;smoke=true;native=true;}
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
    WeatherService weather(!qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS"));
    engine.rootContext()->setContextProperty("Weather",&weather);
    MediaState music;MediaSession session(&music,&audio,nullptr,smoke);auto &player=*session.player();
    MusicLibrary library(&music,&player,nullptr,qEnvironmentVariable("CAR_TERMINAL_MUSIC_LIBRARY_FILE"));
    RadioService radio(&session);VideoLibrary videos(&session,nullptr,qEnvironmentVariable("CAR_TERMINAL_VIDEO_LIBRARY_FILE"));
    AssistantRuntime assistant(*engine.rootContext(),*state,music,session,videos,radio,audio);
    engine.rootContext()->setContextProperty("MediaState",&music);engine.rootContext()->setContextProperty("Media",&session);
    engine.rootContext()->setContextProperty("Radio",&radio);engine.rootContext()->setContextProperty("Videos",&videos);
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
        for(const auto &name:QStringList{"AudioPanel","WeatherPanel","BluetoothPanel","MediaPage","MusicManagerPanel","StatusBar","BottomBar","MediaCard","AssistantPanel","AssistantSettings","AssistantVoicePanel","AssistantAppearancePanel","AssistantDiagnosticsPanel","AssistantWakePanel"}){
            QQmlComponent component(&engine);component.loadFromModule("CarTerminal",name);QVariantMap properties;
            if(name!="AudioPanel"&&name!="WeatherPanel"&&name!="AssistantVoicePanel"&&name!="AssistantAppearancePanel"&&name!="AssistantDiagnosticsPanel"&&name!="AssistantWakePanel")properties["shell"]=QVariant::fromValue(shell.get());
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
    QObject::connect(&music,&MediaState::toast,state,&VehicleState::toast);
    QObject::connect(&session,&MediaSession::notice,&app,[&](const QString &message){emit state->toast(message,"warning");});
    QObject::connect(&videos,&VideoLibrary::notice,&app,[&](const QString &message){emit state->toast(message,"info");});
    QObject::connect(&outputs,&AudioDevices::notice,&app,[&](const QString &message){emit state->toast(message,"info");});
    QObject::connect(&bluetooth,&BluetoothStatus::notice,&app,[&](const QString &message){emit state->toast(message,"warning");});
    QObject::connect(&receiver,&BluetoothAudio::notice,&app,[&](const QString &message){emit state->toast(message,"warning");});
    QObject::connect(&receiver,&BluetoothAudio::changed,&session,[&]{session.setPhoneActive(receiver.active());if(receiver.phase()=="connected"||receiver.phase()=="idle")bluetooth.refresh();});
    QObject::connect(&bluetooth,&BluetoothStatus::changed,&receiver,[&]{const auto status=bluetooth.status()["state"].toString();if(receiver.active()&&(status=="off"||status=="absent"||status=="disabled"))receiver.disconnectDevice();});
    QObject::connect(&outputs,&AudioDevices::changed,&session,[&]{session.setOutputAvailable(outputs.available());receiver.setOutputAvailable(outputs.available());});
    auto videoPolicy=[&]{session.setVideoAllowed(state->gear()=="P"&&state->speed()==0&&state->externalReady());};
    QObject::connect(state,&VehicleState::changed,&session,videoPolicy);QObject::connect(state,&VehicleState::sourceChanged,&session,videoPolicy);videoPolicy();
    session.setOutputAvailable(outputs.available());session.useMusic();
    QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(1);},Qt::QueuedConnection);
    engine.loadFromModule("CarTerminal",app.arguments().contains("--legacy-3d")?"Legacy3D":"Main");if(engine.rootObjects().isEmpty())return 1;
    auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if(mediaSmoke)runMediaSmoke(app,*window,*state,music,session,radio,videos,warnings);else if(musicSmoke)runMusicSmoke(app,*window,music,player,library,warnings);else if(deviceSmoke)runDeviceSmoke(app,*window,music,player,outputs,bluetooth,receiver,warnings);else if(systemSmoke)runSystemSmoke(app,*window,*state,warnings);else if(linkSmoke)runLinkSmoke(app,*window,*state,music,warnings);else if(smoke)runUiSmoke(app,*window,*state,music,player,warnings,native);
    const int result=app.exec();session.stop();session.setVideoOutput(nullptr);player.setAudioBufferOutput(nullptr);player.setAudioOutput(nullptr);videos.save();library.save();
    // Destroy bindings while all context services and the Vehicle singleton live.
    for(auto *root:engine.rootObjects())delete root;
    return result;
}

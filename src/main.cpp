#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFile>
#include "VehicleState.h"
#include "IconProvider.h"
#include "UiSmoke.h"

int main(int argc,char *argv[]) {
    bool smoke=false,native=false;
    for(int i=1;i<argc;++i){const QByteArray argument(argv[i]);if(argument=="--smoke-test")smoke=true;if(argument=="--smoke-native"){smoke=true;native=true;}}
    if(smoke){if(!native)qputenv("QSG_RENDER_LOOP","basic");qputenv("QT_FORCE_STDERR_LOGGING","1");qputenv("CAR_TERMINAL_NO_SETTINGS","1");}
    QGuiApplication app(argc,argv);app.setApplicationName("CarTerminal");app.setOrganizationName("CarTerminalDemo");QQuickStyle::setStyle("Basic");
    QQmlApplicationEngine engine;QStringList warnings;
    QObject::connect(&engine,&QQmlEngine::warnings,&app,[&](const QList<QQmlError> &errors){for(const auto &error:errors)warnings<<error.toString();});
    engine.addImageProvider("icons",new IconProvider);
    engine.rootContext()->setContextProperty("vehicleAssetsAvailable",QFile::exists(":/qt/qml/CarTerminal/assets/sedan/meshes/body_primary_0_mesh.mesh"));
    auto *state=engine.singletonInstance<VehicleState*>("CarTerminal","Vehicle");if(!state)return 1;
    QAudioOutput audio;QMediaPlayer player;player.setAudioOutput(&audio);qint64 pendingSeek=-1;
    auto synchronize=[&]{
        const QUrl source(state->trackUrl());if(player.source()!=source)player.setSource(source);
        audio.setVolume(smoke?0.0f:state->volume()/100.0f);
        if(state->playing())player.play();else player.pause();
    };
    QObject::connect(state,&VehicleState::changed,&app,synchronize);
    QObject::connect(state,&VehicleState::seekRequested,&app,[&](qint64 at){if(player.isSeekable())player.setPosition(at);else pendingSeek=at;});
    QObject::connect(state,&VehicleState::mediaReloadRequested,&app,[&]{player.setSource(QUrl());player.setSource(QUrl(state->trackUrl()));if(state->playing())player.play();});
    QObject::connect(&player,&QMediaPlayer::positionChanged,state,&VehicleState::updatePosition);
    QObject::connect(&player,&QMediaPlayer::durationChanged,state,&VehicleState::updateDuration);
    QObject::connect(&player,&QMediaPlayer::mediaStatusChanged,&app,[&](QMediaPlayer::MediaStatus status){
        if(status==QMediaPlayer::EndOfMedia){state->finishTrack();return;}
        if(status==QMediaPlayer::LoadingMedia)state->updateMediaStatus("loading");
        if(status==QMediaPlayer::LoadedMedia||status==QMediaPlayer::BufferedMedia){state->updateMediaStatus("ready");if(pendingSeek>=0){player.setPosition(pendingSeek);pendingSeek=-1;}}
    });
    QObject::connect(&player,&QMediaPlayer::errorOccurred,&app,[&](QMediaPlayer::Error,const QString &message){state->updateMediaStatus("error",message);emit state->toast(QStringLiteral("音源加载失败，可在音乐页重试"),"warning");});
    synchronize();
    QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[]{QCoreApplication::exit(1);},Qt::QueuedConnection);
    engine.loadFromModule("CarTerminal",app.arguments().contains("--legacy-3d")?"Legacy3D":"Main");if(engine.rootObjects().isEmpty())return 1;
    auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first());
    if(smoke)runUiSmoke(app,*window,*state,player,warnings,native);
    return app.exec();
}

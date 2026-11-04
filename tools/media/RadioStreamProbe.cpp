#include <QGuiApplication>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QAudioBufferOutput>
#include <QAudioBuffer>
#include <QElapsedTimer>
#include <QThread>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QTimeZone>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <functional>

// Bounded, muted capability experiment; independent of the production player.
int main(int argc,char **argv){
    qputenv("QT_QPA_PLATFORM","offscreen");QGuiApplication app(argc,argv);
    const auto args=app.arguments();if(args.size()!=3){qCritical()<<"RadioStreamProbe sources.json report.json";return 2;}
    QFile input(args[1]);if(!input.open(QIODevice::ReadOnly))return 2;
    QJsonParseError parse;const auto sources=QJsonDocument::fromJson(input.readAll(),&parse).object()["stations"].toArray();
    if(parse.error!=QJsonParseError::NoError||sources.isEmpty()||sources.size()>10)return 2;
    auto wait=[&](const std::function<bool()> &ready,int limit){QElapsedTimer t;t.start();while(!ready()&&t.elapsed()<limit){app.processEvents();QThread::msleep(10);}return ready();};
    QMediaPlayer player;QAudioOutput output;output.setMuted(true);player.setAudioOutput(&output);
    QAudioBufferOutput decoded;player.setAudioBufferOutput(&decoded);qint64 frames=0;int buffers=0;
    QObject::connect(&decoded,&QAudioBufferOutput::audioBufferReceived,&app,[&](const QAudioBuffer &buffer){if(buffer.isValid()){frames+=buffer.frameCount();++buffers;}});
    QJsonArray rows;bool all=true;
    for(const auto &value:sources){
        const auto source=value.toObject();const QUrl url(source["url"].toString());
        if(url.scheme()!="https"&&url.scheme()!="http")return 2;
        frames=0;buffers=0;QJsonArray states;
        const auto statusConnection=QObject::connect(&player,&QMediaPlayer::mediaStatusChanged,&app,[&](QMediaPlayer::MediaStatus s){states.append(int(s));});
        QElapsedTimer elapsed;elapsed.start();player.setSource(url);player.play();
        wait([&]{return (frames>=20000&&buffers>=5)||player.error()!=QMediaPlayer::NoError;},22000);
        const bool audioDecoded=frames>=20000&&buffers>=5;const auto firstDecodeMs=elapsed.elapsed();const auto error=player.errorString();const auto frameCount=frames;const auto bufferCount=buffers;
        const bool seekable=player.isSeekable();player.stop();player.setSource(QUrl());
        wait([&]{return player.playbackState()==QMediaPlayer::StoppedState;},1500);
        // Drain already queued callbacks, then prove decoded output stays stopped.
        QElapsedTimer drain;drain.start();wait([&]{return drain.elapsed()>=250;},350);const auto settledFrames=frames;
        QElapsedTimer quiet;quiet.start();wait([&]{return quiet.elapsed()>=600;},700);
        const bool stopped=player.playbackState()==QMediaPlayer::StoppedState&&player.source().isEmpty()&&frames==settledFrames;
        QObject::disconnect(statusConnection);const bool passed=audioDecoded&&stopped;all&=passed;
        rows.append(QJsonObject{{"id",source["id"]},{"name",source["name"]},{"originPage",source["originPage"]},{"transport","HLS"},{"host",url.host()},{"temporaryQuery",url.hasQuery()},
            {"decodedFrames",frameCount},{"decodedBuffers",bufferCount},{"decodedAudio",audioDecoded},{"decodeWaitMs",firstDecodeMs},{"seekableReported",seekable},{"stoppedAndCleared",stopped},{"mediaStates",states},{"error",error},{"passed",passed}});
        qInfo().noquote()<<source["id"].toString()<<"decoded="<<audioDecoded<<"stopped="<<stopped<<error;
    }
    // Controlled local fixtures distinguish cancellation/error behavior from CDN availability.
    QTcpServer server;bool fixtureReady=server.listen(QHostAddress::LocalHost,0);int accepted=0;
    QObject::connect(&server,&QTcpServer::newConnection,&app,[&]{while(server.hasPendingConnections()){
        auto *socket=server.nextPendingConnection();++accepted;
        QObject::connect(socket,&QTcpSocket::readyRead,socket,[socket]{const auto request=socket->readAll();if(request.contains("/missing")){socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");socket->disconnectFromHost();}});
        QObject::connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
    }});
    bool cancelled=false,failed=false;
    if(fixtureReady){
        const QString address="http://127.0.0.1:"+QString::number(server.serverPort());
        player.setSource(QUrl(address+"/hold"));player.play();wait([&]{return accepted>0;},3000);
        QElapsedTimer cancel;cancel.start();player.stop();player.setSource(QUrl());
        cancelled=wait([&]{return player.playbackState()==QMediaPlayer::StoppedState&&player.source().isEmpty();},1500)&&accepted>0&&cancel.elapsed()<1500;
        for(auto *socket:server.findChildren<QTcpSocket*>())socket->abort();
        player.setSource(QUrl(address+"/missing"));player.play();failed=wait([&]{return player.error()!=QMediaPlayer::NoError;},5000)&&player.error()!=QMediaPlayer::NoError;
        player.stop();player.setSource(QUrl());
    }
    all&=fixtureReady&&cancelled&&failed;
    QDir().mkpath(QFileInfo(args[2]).absolutePath());QFile report(args[2]);if(!report.open(QIODevice::WriteOnly))return 2;
    report.write(QJsonDocument(QJsonObject{{"capturedAt",QDateTime::currentDateTime().toTimeZone(QTimeZone("Asia/Shanghai")).toString(Qt::ISODate)},
        {"audioMuted",true},{"audioRecorded",false},{"productionIntegrated",false},{"stations",rows},{"fixtureServerStarted",fixtureReady},{"pendingLoadCancelled",cancelled},{"http404Reported",failed},{"allPassed",all}}).toJson());
    return all?0:1;
}

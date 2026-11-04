#include "media/MediaSession.h"
#include "media/MediaState.h"
#include <QGuiApplication>
#include <QAudioOutput>
#include <QAudioBufferOutput>
#include <QAudioBuffer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QDirIterator>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("CAR_TERMINAL_NO_SETTINGS", "1");
    QGuiApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 3)
        return 2;
    QStringList files;
    QDirIterator paths(args[1], {"*.mp4", "*.avi", "*.mkv", "*.webm"}, QDir::Files,
                       QDirIterator::Subdirectories);
    while (paths.hasNext())
        files << paths.next();
    if (files.isEmpty() || files.size() > 40)
        return 2;
    MediaState music;
    QAudioOutput output;
    MediaSession session(&music, &output, nullptr, true);
    QVideoSink sink;
    session.setVideoOutput(&sink);
    QAudioBufferOutput decoded;
    session.player()->setAudioBufferOutput(&decoded);
    int frames = 0;
    qint64 audioFrames = 0;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &app, [&](const QVideoFrame &frame) {
        if (frame.isValid())
            ++frames;
    });
    QObject::connect(&decoded, &QAudioBufferOutput::audioBufferReceived, &app,
                     [&](const QAudioBuffer &buffer) {
                         if (buffer.isValid())
                             audioFrames += buffer.frameCount();
                     });
    auto wait = [&](auto ready, int limit) {
        QElapsedTimer elapsed;
        elapsed.start();
        while (!ready() && elapsed.elapsed() < limit) {
            app.processEvents();
            QThread::msleep(10);
        }
        return ready();
    };
    QJsonArray rows;
    bool all = true;
    for (const auto &file : files) {
        session.player()->setAudioBufferOutput(nullptr);
        session.stop();
        app.processEvents();
        frames = 0;
        audioFrames = 0;
        session.player()->setAudioBufferOutput(&decoded);
        session.playVideo(QUrl::fromLocalFile(QFileInfo(file).absoluteFilePath()),
                          QFileInfo(file).fileName());
        wait(
            [&] {
                return (frames >= 3 && (!session.hasAudio() || audioFrames > 0)) ||
                       session.phase() == "error";
            },
            8000);
        const auto duration = session.duration();
        const bool video = frames >= 3, audio = !session.hasAudio() || audioFrames > 0;
        session.pause();
        const qint64 target = qint64(duration / 3);
        session.seek(target);
        const bool seek = wait([&] { return qAbs(session.position() - target) < 180; }, 3000);
        session.toggle();
        const bool resumed = wait([&] { return session.position() > target + 70; }, 4000);
        session.pause();
        const bool passed = video && audio && seek && resumed;
        all &= passed;
        rows << QJsonObject{{"file", QFileInfo(file).fileName()},
                            {"durationMs", duration},
                            {"videoFrames", frames},
                            {"audioFrames", audioFrames},
                            {"hasAudio", session.hasAudio()},
                            {"seekPassed", seek},
                            {"resumed", resumed},
                            {"error", session.error()},
                            {"passed", passed}};
        session.stop();
    }
    QDir().mkpath(QFileInfo(args[2]).absolutePath());
    QFile report(args[2]);
    if (!report.open(QIODevice::WriteOnly))
        return 2;
    report.write(QJsonDocument(QJsonObject{{"productionMediaSession", true},
                                           {"audioMuted", true},
                                           {"allPassed", all},
                                           {"files", rows}})
                     .toJson());
    return all ? 0 : 1;
}

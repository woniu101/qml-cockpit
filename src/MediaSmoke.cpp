#include "MediaSmoke.h"
#include "VehicleState.h"
#include "media/MediaState.h"
#include "media/MediaSession.h"
#include "media/RadioService.h"
#include "media/VideoLibrary.h"
#include "VideoFixture.h"
#include <QGuiApplication>
#include <QQuickWindow>
#include <QQuickItem>
#include <QVideoSink>
#include <QVideoFrame>
#include <QAudioBufferOutput>
#include <QAudioBuffer>
#include <QAudioOutput>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QtTest>
#include <functional>
#include <stdexcept>

void runMediaSmoke(QGuiApplication &app, QQuickWindow &window, VehicleState &vehicle, MediaState &music,
                   MediaSession &media, RadioService &radio, VideoLibrary &videos, QStringList &warnings) {
    window.setProperty("testMode", true);
    window.setProperty("clockText", "16:28");
    QTimer::singleShot(1100, &app, [&] {
        const auto directory = qEnvironmentVariable("CAR_TERMINAL_MEDIA_OUTPUT", "artifacts/media/native");
        QDir().mkpath(directory);
        window.raise();
        window.requestActivate();
        auto require = [](bool ok, const char *reason) {
            if (!ok)
                throw std::runtime_error(reason);
        };
        std::function<QQuickItem *(QQuickItem *, const QString &)> find;
        find = [&](QQuickItem *p, const QString &name) -> QQuickItem * {
            if (p->objectName() == name && p->isVisible())
                return p;
            for (auto *child : p->childItems())
                if (auto *found = find(child, name))
                    return found;
            return nullptr;
        };
        auto item = [&](const char *name) {
            auto *value = find(window.contentItem(), name);
            require(value, name);
            return value;
        };
        auto click = [&](const char *name) {
            auto *target = item(name);
            require(target->isEnabled(), name);
            QTest::mouseClick(
                &window, Qt::LeftButton, Qt::NoModifier,
                target->mapToScene(QPointF(target->width() / 2, target->height() / 2)).toPoint());
            QTest::qWait(100);
        };
        auto wait = [&](std::function<bool()> ready, const char *reason, int limit = 12000) {
            QElapsedTimer elapsed;
            elapsed.start();
            while (!ready() && elapsed.elapsed() < limit)
                QTest::qWait(20);
            require(ready(), reason);
        };
        auto capture = [&](const char *name) {
            window.setProperty("toastMessage", "");
            QTest::qWait(220);
            require(window.grabWindow().save(directory + "/" + name + ".png"), "media screenshot");
        };
        try {
            click("tab3");
            click("mediaRadioTab");
            capture("01-radio-idle");
            click("radioFavorite");
            require(radio.currentFavorite(), "favorite current station");
            radio.setFilter("favorites");
            require(radio.rowCount() == 1, "favorites model");
            radio.setFilter("all");
            QAudioBufferOutput decoded;
            qint64 audioFrames = 0;
            QObject::connect(&decoded, &QAudioBufferOutput::audioBufferReceived, &decoded,
                             [&](const QAudioBuffer &buffer) { audioFrames += buffer.frameCount(); });
            const bool outputAvailable = !QMediaDevices::audioOutputs().isEmpty();
            if (outputAvailable) {
                media.player()->setAudioBufferOutput(&decoded);
                click("radioPlayStop");
                capture("02-radio-connecting");
                wait([&] { return audioFrames > 20000 && media.playing(); },
                     "production radio service resolves and decodes official stream", 35000);
                require(!media.canSeek(), "live radio has no seek");
                capture("03-radio-playing");
                click("tab0");
                require(media.kind() == "radio" && media.playing(), "radio continues on home");
                capture("04-radio-home");
                click("homeRadio");
                require(!media.playing() && media.player()->source().isEmpty(),
                        "home stop clears live player");
                click("tab3");
                click("mediaRadioTab");
                radio.tune("cnr-2");
                radio.stop();
                const auto stopped = media.generation();
                QTest::qWait(600);
                require(media.generation() == stopped && !media.playing(),
                        "cancelled catalog request does not resurrect playback");
            } else {
                require(!item("radioPlayStop")->isEnabled(), "no-output UI disables radio playback");
                const auto generation = media.generation();
                radio.tune("cnr-1");
                require(media.generation() == generation && !media.playing(),
                        "central guard rejects audible playback without an output");
                capture("02-radio-no-output");
                // Decode-only validation uses a separate production session, never a fake desktop device.
                MediaState probeMusic;
                QAudioOutput probeOutput;
                MediaSession probe(&probeMusic, &probeOutput, nullptr, true);
                RadioService probeRadio(&probe);
                QAudioBufferOutput probeDecoded;
                QObject::connect(&probeDecoded, &QAudioBufferOutput::audioBufferReceived, &probeDecoded,
                                 [&](const QAudioBuffer &buffer) { audioFrames += buffer.frameCount(); });
                probe.player()->setAudioOutput(nullptr);
                probe.player()->setAudioBufferOutput(&probeDecoded);
                probeRadio.tune("cnr-1");
                wait([&] { return audioFrames > 20000 && probe.playing(); },
                     "production radio decode-only sink without physical output", 35000);
                probeRadio.stop();
                require(probe.player()->source().isEmpty(), "decode-only radio stop clears source");
                probeRadio.tune("cnr-2");
                probeRadio.stop();
                const auto generationAfterStop = probe.generation();
                QTest::qWait(600);
                require(probe.generation() == generationAfterStop && !probe.playing(),
                        "cancelled decode-only request remains stopped");
            }
            click("mediaVideoTab");
            capture("05-video-empty");
            const QString root = QFileInfo(directory + "/fixtures").absoluteFilePath(),
                          clip = root + "/风景/动态场景.avi";
            require(writeVideoFixture(clip), "original AVI fixture");
            videos.importFolder(QUrl::fromLocalFile(root));
            wait([&] { return !videos.scanning() && videos.count() == 1; },
                 "recursive video scan and preview");
            capture("06-video-library");
            click("videoTile0");
            wait(
                [&] {
                    return media.kind() == "video" && media.player()->duration() > 17000 && media.playing();
                },
                "real video playback");
            auto *sink = item("videoOutput")->property("videoSink").value<QVideoSink *>();
            require(sink, "QML video sink");
            wait([&] { return sink->videoFrame().isValid(); }, "actual decoded frames reach QML VideoOutput");
            require(!item("videoVolume")->isEnabled(),
                    "silent video does not offer an audible volume control");
            click("videoPlaylistToggle");
            require(item("videoFrame")->width() > 1400, "video list can collapse");
            click("videoPlaylistToggle");
            media.seek(6000);
            wait([&] { return media.position() >= 5900; }, "video seek");
            click("videoForward10");
            wait([&] { return media.position() > 15500; }, "forward ten seconds");
            click("videoBack10");
            wait([&] { return media.position() < 9000; }, "back ten seconds");
            capture("07-video-player");
            click("videoFullscreen");
            require(media.fullscreen(), "fullscreen enabled");
            capture("08-video-fullscreen");
            click("videoFullscreen");
            require(!media.fullscreen(), "fullscreen exits");
            vehicle.startDriving();
            wait([&] { return !media.videoAllowed() && !media.playing(); }, "D gear pauses video");
            require(media.phase() == "blocked", "blocked state is visible");
            capture("09-video-driving-restriction");
            vehicle.parkVehicle();
            require(!media.playing(), "parking does not auto-resume");
            click("videoPlayPause");
            wait([&] { return media.playing(); }, "manual resume after parking");
            click("tab0");
            require(!media.playing(), "leaving video pauses");
            capture("10-video-home-paused");
            click("tab3");
            click("mediaVideoTab");
            window.resize(1120, 630);
            QTest::qWait(180);
            capture("11-media-minimum");
            click("mediaRadioTab");
            vehicle.setNight(true);
            capture("12-radio-night-minimum");
            vehicle.setNight(false);
            media.player()->setAudioBufferOutput(nullptr);
            media.stop();
            require(warnings.isEmpty(), qPrintable(warnings.join('\n')));
            QFile report(directory + "/verification.json");
            require(report.open(QIODevice::WriteOnly), "media report");
            report.write(
                QJsonDocument(
                    QJsonObject{{"radioDecodedFrames", audioFrames},
                                {"capturedAt", QDateTime::currentDateTime().toString(Qt::ISODate)},
                                {"audioOutputAvailable", outputAvailable},
                                {"radioValidation",
                                 outputAvailable
                                     ? "muted UI playback"
                                     : "no-output UI guard + separate decode-only production session"},
                                {"liveSource", "official CNR catalog, production resolver"},
                                {"audioMuted", true},
                                {"qmlVideoFrames", true},
                                {"videoFixture", "original generated AVI"},
                                {"sourceSwitching", true},
                                {"lateRequestIgnored", true},
                                {"parkingGate", true},
                                {"fullscreen", true},
                                {"warnings", QJsonArray::fromStringList(warnings)}})
                    .toJson());
            qInfo() << "PASS media center: production live radio, cancellation, real video frames/seek, "
                       "source switching, parking gate and fullscreen";
            app.exit(0);
        } catch (const std::exception &error) {
            media.stop();
            window.grabWindow().save(directory + "/failure.png");
            qCritical() << "MEDIA UI FAIL:" << error.what();
            app.exit(2);
        }
    });
}

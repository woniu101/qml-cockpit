#include <QtTest>
#include <algorithm>
#include <QTemporaryDir>
#include <QAudioOutput>
#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QFile>
#include <QVideoSink>
#include <QVideoFrame>
#include <QSettings>
#include <QScopeGuard>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QProcess>
#include <QUuid>
#include "media/MediaState.h"
#include "media/MediaSession.h"
#include "media/VideoLibrary.h"
#include "media/RadioService.h"
#include "media/MediaDirectory.h"
#include "MusicFixture.h"
#include "MusicLibrary.h"
#include "VideoFixture.h"
#include "assistant/AssistantMedia.h"
#include "assistant/AssistantSpeech.h"
#include "assistant/AssistantVehicle.h"
#include "assistant/AssistantNlu.h"
#include "assistant/AssistantAppearance.h"
#include "assistant/AssistantWake.h"
#include "VehicleState.h"
#include "protocol/VehicleProtocol.h"

class MediaSessionTests : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { qputenv("CAR_TERMINAL_NO_SETTINGS", "1"); }
    void assistantCustomWakeReplaysWithoutListening() {
        AssistantWake wake;
        QVERIFY(!wake.enabled());
        QVERIFY(!wake.listening());
        QCOMPARE(wake.keyword(), QStringLiteral("你好小林"));
        const auto recordings = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_WAKE_WAVS");
        if (recordings.isEmpty() || !wake.configured())
            QSKIP("Supply KWS worker/model and offline wake samples");
        QSignalSpy activated(&wake, &AssistantWake::activated);
        auto replay = [&](const QString &sample) {
            wake.replay(QUrl::fromLocalFile(recordings + '/' + sample + ".wav"));
            QTRY_VERIFY_WITH_TIMEOUT(!wake.testing(), 12000);
        };
        replay("wake-0-1");
        QVERIFY(wake.testResult().startsWith(QStringLiteral("WAV 命中")));
        wake.saveKeyword(QStringLiteral("小林同学"));
        QTRY_VERIFY_WITH_TIMEOUT(!wake.testing(), 12000);
        QCOMPARE(wake.keyword(), QStringLiteral("小林同学"));
        replay("wake-1-1");
        QVERIFY(wake.testResult().startsWith(QStringLiteral("WAV 命中")));
        replay("wake-0-1");
        QVERIFY(wake.testResult().startsWith(QStringLiteral("WAV 未命中")));
        replay("command-00");
        QVERIFY(wake.testResult().startsWith(QStringLiteral("WAV 未命中")));
        wake.saveKeyword(QStringLiteral("小林"));
        QTRY_VERIFY_WITH_TIMEOUT(!wake.testing(), 12000);
        QCOMPARE(wake.keyword(), QStringLiteral("小林同学"));
        QVERIFY(wake.testResult().contains(QStringLiteral("4 到 8")));
        QVERIFY(!wake.listening());
        QVERIFY(activated.isEmpty());
    }
    void assistantAvatarCropsAndPersistsWithoutTouchingVoice() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QImage original(300, 100, QImage::Format_RGB32);
        original.fill(Qt::red);
        for (int y = 0; y < original.height(); ++y)
            for (int x = 200; x < original.width(); ++x)
                original.setPixelColor(x, y, Qt::blue);
        const auto input = dir.filePath("wide.png");
        const auto storage = dir.filePath("settings/avatar.png");
        QVERIFY(original.save(input));
        AssistantAppearance avatar(storage);
        QVERIFY(avatar.load(QUrl::fromLocalFile(input)));
        avatar.crop(1, 100, 50);
        QCOMPARE(QImage(avatar.preview().toLocalFile()).pixelColor(190, 190), QColor(Qt::blue));
        QVERIFY(!avatar.custom());
        QVERIFY(avatar.save());
        AssistantAppearance reloaded(storage);
        QVERIFY(reloaded.custom());
        QCOMPARE(QImage(reloaded.avatar().toLocalFile()).size(), QSize(384, 384));
        QVERIFY(!reloaded.load(QUrl("https://example.invalid/avatar.jpg")));
        QVERIFY(reloaded.custom());
        reloaded.restoreDefault();
        QVERIFY(!reloaded.custom());
        QCOMPARE(reloaded.avatar(), QUrl("qrc:/assistant/character.png"));
    }
    void assistantResolvesDuplicatesAndKeepsRepeatPlaying() {
        QTemporaryDir dir;
        const QString a = dir.filePath("a.wav"), b = dir.filePath("b.wav");
        QVERIFY(writeMusicFixture(a));
        QVERIFY(writeMusicFixture(b));
        MediaState music;
        QAudioOutput audio;
        QAudioBufferOutput buffer;
        MediaSession session(&music, &audio, nullptr, true);
        session.player()->setAudioOutput(nullptr);
        session.player()->setAudioBufferOutput(&buffer);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos);
        music.mergeMusicTracks({QVariantMap{{"url", QUrl::fromLocalFile(a).toString()}, {"title", "同名歌曲"}, {"artist", "歌手甲"}},
                                QVariantMap{{"url", QUrl::fromLocalFile(b).toString()}, {"title", "同名歌曲"}, {"artist", "歌手乙"}}});
        QSignalSpy played(&assistant, &AssistantMedia::viewRequested);
        assistant.submit(QStringLiteral("播放同名歌曲"));
        QCOMPARE(assistant.choices().size(), 2);
        QVERIFY(!session.playing());
        const int expected = assistant.choices().at(0).toMap().value("url").toString() == QUrl::fromLocalFile(b).toString() ? 1 : 2;
        QVERIFY(assistant.chooseByRequest(QStringLiteral("选第%1个").arg(expected), false));
        QTRY_COMPARE_WITH_TIMEOUT(played.size(), 1, 6000);
        QCOMPARE(music.trackUrl(), QUrl::fromLocalFile(b).toString());
        assistant.submit(QStringLiteral("播放歌手乙的同名歌曲"));
        QVERIFY(session.playing());
        assistant.submit(QStringLiteral("别播放同名歌曲"));
        QCOMPARE(music.trackUrl(), QUrl::fromLocalFile(b).toString());
        QVERIFY(session.playing());
        assistant.submit(QStringLiteral("暂停音乐"));
        QVERIFY(!session.playing());
        assistant.submit(QStringLiteral("继续播放音乐"));
        QTRY_VERIFY(session.playing());
        assistant.submit(QStringLiteral("上一首"));
        QTRY_COMPARE(music.trackUrl(), QUrl::fromLocalFile(a).toString());
        assistant.submit(QStringLiteral("下一首"));
        QTRY_COMPARE(music.trackUrl(), QUrl::fromLocalFile(b).toString());
        assistant.submit(QStringLiteral("音量调到百分之三十"));
        QCOMPARE(music.volume(), 30);
        assistant.submit(QStringLiteral("音量提高10%"));
        QCOMPARE(music.volume(), 40);
        assistant.submit(QStringLiteral("别把音量调到80%"));
        QCOMPARE(music.volume(), 40);
        assistant.submit(QStringLiteral("音量调到150%"));
        QCOMPARE(music.volume(), 40);
        QTRY_VERIFY_WITH_TIMEOUT(session.canSeek() && !session.busy() && session.duration() > 10000, 6000);
        assistant.submit(QStringLiteral("暂停音乐"));
        const auto before = session.position();
        assistant.submit(QStringLiteral("快进三秒"));
        QTRY_VERIFY_WITH_TIMEOUT(assistant.message().contains(QStringLiteral("已跳到")), 5000);
        QVERIFY(std::abs(session.position() - before - 3000) < 450);
        assistant.submit(QStringLiteral("快退两秒"));
        QTRY_VERIFY_WITH_TIMEOUT(assistant.message().contains(QStringLiteral("已跳到")), 5000);
        QVERIFY(std::abs(session.position() - before - 1000) < 450);
        const auto clip = dir.filePath(QStringLiteral("同名歌曲MV.avi"));
        QVERIFY(writeVideoFixture(clip));
        videos.importFiles({QUrl::fromLocalFile(clip)});
        QTRY_VERIFY_WITH_TIMEOUT(!videos.scanning(), 6000);
        assistant.submit(QStringLiteral("播放同名歌曲"));
        QCOMPARE(assistant.choices().size(), 3);
        QCOMPARE(session.kind(), "music");
        assistant.submit(QStringLiteral("播放不存在的歌"));
        QVERIFY(assistant.message().contains(QStringLiteral("没有找到")));
        session.stop();
    }
    void assistantReplaysSuppliedMedia() {
        const QString song = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_TEST_SONG");
        const QString video = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_TEST_VIDEO");
        const QString recordings = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_TEST_WAVS");
        if (song.isEmpty() || video.isEmpty() || recordings.isEmpty())
            QSKIP("Supply real media and synthetic WAV paths to exercise the offline ASR replay");
        MediaState music;
        QAudioOutput audio;
        QAudioBufferOutput buffer;
        QVideoSink sink;
        MediaSession session(&music, &audio, nullptr, true);
        session.player()->setAudioOutput(nullptr);
        session.player()->setAudioBufferOutput(&buffer);
        session.setVideoOutput(&sink);
        MusicLibrary library(&music, session.player());
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos);
        AssistantSpeech speech(audio);
        QVERIFY(speech.configured());
        library.importFiles({QUrl::fromLocalFile(song)});
        videos.importFiles({QUrl::fromLocalFile(video)});
        QTRY_VERIFY_WITH_TIMEOUT(!library.scanning() && !videos.scanning(), 15000);
        QVERIFY(!videos.playbackItems().isEmpty());
        QSignalSpy spoken(&speech, &AssistantSpeech::recognized);
        QSignalSpy played(&assistant, &AssistantMedia::viewRequested);
        connect(&speech, &AssistantSpeech::recognized, &assistant, [&](const QString &text) { assistant.submit(text); });
        auto replay = [&](const QString &name) {
            speech.openWave(QUrl::fromLocalFile(recordings + '/' + name + ".wav"));
            return speech.recognizing();
        };
        speech.openWave(QUrl("https://invalid.example/song.wav"));
        QVERIFY(!speech.recognizing());
        QVERIFY(speech.message().contains(QStringLiteral("本地 WAV")));
        QSignalSpy decoded(&buffer, &QAudioBufferOutput::audioBufferReceived);
        QVERIFY(replay("song"));
        QTRY_COMPARE_WITH_TIMEOUT(spoken.size(), 1, 15000);
        qInfo() << "Song transcript:" << spoken.last().at(0).toString() << assistant.message();
        QTRY_COMPARE_WITH_TIMEOUT(played.size(), 1, 10000);
        QCOMPARE(music.trackUrl(), QUrl::fromLocalFile(song).toString());
        QTRY_VERIFY_WITH_TIMEOUT(std::any_of(decoded.cbegin(), decoded.cend(), [](const auto &event) {
            return event.at(0).template value<QAudioBuffer>().isValid();
        }), 5000);
        QVERIFY(replay("negative-song"));
        QTRY_COMPARE_WITH_TIMEOUT(spoken.size(), 2, 15000);
        qInfo() << "Negative song transcript:" << spoken.last().at(0).toString();
        QCOMPARE(played.size(), 1);
        QVERIFY(replay("negative-ac"));
        QTRY_COMPARE_WITH_TIMEOUT(spoken.size(), 3, 15000);
        qInfo() << "Negative AC transcript:" << spoken.last().at(0).toString();
        QCOMPARE(played.size(), 1);
        session.setVideoAllowed(false);
        assistant.submit(QStringLiteral("播放七里香MV视频"));
        QVERIFY(assistant.message().contains(QStringLiteral("驻车")));
        QCOMPARE(session.kind(), "music");
        session.setVideoAllowed(true);
        QVERIFY(replay("video"));
        QTRY_COMPARE_WITH_TIMEOUT(spoken.size(), 4, 15000);
        qInfo() << "Video transcript:" << spoken.last().at(0).toString() << assistant.message();
        QTRY_COMPARE_WITH_TIMEOUT(played.size(), 2, 10000);
        QCOMPARE(session.itemId(), QUrl::fromLocalFile(video).toString());
        QTRY_VERIFY_WITH_TIMEOUT(sink.videoFrame().isValid(), 6000);
        session.stop();
        session.setVideoOutput(nullptr);
        session.player()->setAudioBufferOutput(nullptr);
    }
    void assistantReplaysVehicleCommands() {
        const auto recordings = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_CONTROL_WAVS");
        if (recordings.isEmpty())
            QSKIP("Supply the vehicle-command recordings to exercise offline recognition");
        VehicleState vehicle;
        AssistantVehicle commands(vehicle);
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos, [&](const QString &request, bool spoken, AssistantMedia::Reply reply) {
            return commands.submit(request, std::move(reply), spoken);
        });
        AssistantSpeech speech(audio);
        QVERIFY(speech.configured());
        connect(&speech, &AssistantSpeech::recognized, &assistant, [&](const QString &text) {
            qInfo() << "Vehicle transcript:" << text;
            assistant.submit(text, true);
        });
        QSignalSpy replies(&assistant, &AssistantMedia::spokenReply);
        auto replay = [&](const QString &file) {
            speech.openWave(QUrl::fromLocalFile(recordings + '/' + file));
        };
        vehicle.beginWindowDrag(0);
        vehicle.previewWindow(0, 70);
        vehicle.endWindowDrag(0);
        replay("negative-window.wav");
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 1, 15000);
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 70.0);
        replay("close-window.wav");
        QTRY_COMPARE_WITH_TIMEOUT(assistant.message(), QStringLiteral("正在关闭左前车窗…"), 15000);
        QCOMPARE(replies.size(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 2, 3500);
        QCOMPARE(replies.last().at(0).toString(), QStringLiteral("左前车窗已关闭"));
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 0.0);
        replay("driver-full.wav");
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 3, 15000);
        QCOMPARE(replies.last().at(0).toString(), QStringLiteral("主驾空调已设为 23 度"));
        QCOMPARE(vehicle.driverTemperature(), 23.0);
        vehicle.setDriverTemperature(21);
        replay("driver-short.wav");
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 4, 15000);
        QCOMPARE(replies.last().at(0).toString(), QStringLiteral("主驾空调已设为 23 度"));
        QCOMPARE(vehicle.driverTemperature(), 23.0);
    }
    void assistantReplaysExtendedVehicleCommands() {
        const auto recordings = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_EXTENDED_WAVS");
        if (recordings.isEmpty())
            QSKIP("Supply the extended vehicle-command recordings");
        VehicleState vehicle;
        AssistantVehicle commands(vehicle);
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos, [&](const QString &request, bool spoken, AssistantMedia::Reply reply) {
            return commands.submit(request, std::move(reply), spoken);
        });
        AssistantSpeech speech(audio);
        QVERIFY(speech.configured());
        connect(&speech, &AssistantSpeech::recognized, &assistant, [&](const QString &text) { assistant.submit(text, true); });
        QSignalSpy replies(&assistant, &AssistantMedia::spokenReply);
        auto replay = [&](const QString &file) { speech.openWave(QUrl::fromLocalFile(recordings + '/' + file)); };
        replay("right-rear.wav");
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 1, 15000);
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 30.0);
        replay("all-windows.wav");
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 2, 15000);
        QCOMPARE(assistant.message(), QStringLiteral("全部车窗已关闭"));
        replay("passenger.wav");
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 3, 15000);
        QCOMPARE(vehicle.passengerTemperature(), 24.0);
        replay("fan-full.wav");
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 4, 15000);
        QCOMPARE(vehicle.fan(), 3);
        vehicle.setAcOn(true);
        replay("cooling.wav");
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(), 5, 15000);
        QVERIFY(!vehicle.acOn());
    }
    void assistantClarificationWaitsAndExpires() {
        const auto original = qgetenv("CAR_TERMINAL_NLU_MODEL");
        qunsetenv("CAR_TERMINAL_NLU_MODEL");
        const auto restore = qScopeGuard([&] {
            if (!original.isNull())
                qputenv("CAR_TERMINAL_NLU_MODEL", original);
        });
        VehicleState vehicle;
        AssistantVehicle controls(vehicle);
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos, [&](const QString &text, bool spoken, AssistantMedia::Reply reply) {
            return controls.submit(text, std::move(reply), spoken);
        });
        AssistantNlu nlu(vehicle, controls, assistant, 750);
        nlu.submit(QStringLiteral("空调调到 24 度"));
        QCOMPARE(nlu.followUpOptions(), (QStringList{QStringLiteral("主驾"), QStringLiteral("副驾")}));
        QCOMPARE(vehicle.driverTemperature(), 22.0);
        QCOMPARE(vehicle.passengerTemperature(), 22.0);
        const int zoneRevision = nlu.followUpRevision();
        nlu.chooseFollowUp(1, zoneRevision);
        QCOMPARE(vehicle.passengerTemperature(), 24.0);
        QCOMPARE(vehicle.driverTemperature(), 22.0);
        QVERIFY(nlu.followUpOptions().isEmpty());
        nlu.chooseFollowUp(0, zoneRevision);
        QCOMPARE(vehicle.driverTemperature(), 22.0);

        nlu.submit(QStringLiteral("把车窗开到 30%"));
        QCOMPARE(nlu.followUpOptions().size(), 4);
        nlu.submit(QStringLiteral("取消"));
        QVERIFY(nlu.followUpOptions().isEmpty());
        nlu.submit(QStringLiteral("右后窗"));
        QCOMPARE(vehicle.windowTargets().at(3).toInt(), 0);
        nlu.submit(QStringLiteral("把车窗开到 30%"));
        QTRY_VERIFY_WITH_TIMEOUT(nlu.followUpOptions().isEmpty(), 1800);
        QVERIFY(assistant.message().contains(QStringLiteral("超时")));
        nlu.submit(QStringLiteral("把车窗开到 30%"));
        nlu.endConversation();
        nlu.submit(QStringLiteral("右后窗"));
        QCOMPARE(vehicle.windowTargets().at(3).toInt(), 0);
        nlu.submit(QStringLiteral("把车窗开到 30%"));
        nlu.submit(QStringLiteral("现在还有多少电"));
        QVERIFY(nlu.followUpOptions().isEmpty());
        QVERIFY(assistant.message().contains(QStringLiteral("82%")));
        nlu.submit(QStringLiteral("副驾调到 26 度，算了"));
        QCOMPARE(vehicle.passengerTemperature(), 24.0);
        nlu.submit(QStringLiteral("空调调到 31 度"));
        QVERIFY(nlu.followUpOptions().isEmpty());
        QVERIFY(assistant.message().contains(QStringLiteral("范围")));

        nlu.submit(QStringLiteral("再低一度"));
        QCOMPARE(nlu.followUpOptions().size(), 2);
        QCOMPARE(vehicle.passengerTemperature(), 24.0);
        nlu.submit(QStringLiteral("副驾驶"));
        QCOMPARE(vehicle.passengerTemperature(), 23.0);
        vehicle.setPassengerTemperature(25.0);
        nlu.submit(QStringLiteral("再低一点"));
        QCOMPARE(vehicle.passengerTemperature(), 24.5);
        QCOMPARE(vehicle.driverTemperature(), 22.0);
        nlu.submit(QStringLiteral("再低二十度"));
        QVERIFY(assistant.message().contains(QStringLiteral("调整量")));
        QCOMPARE(vehicle.passengerTemperature(), 24.5);

        nlu.submit(QStringLiteral("主驾调到 29.5 度"));
        nlu.submit(QStringLiteral("再高一度"));
        QCOMPARE(vehicle.driverTemperature(), 30.0);
        nlu.submit(QStringLiteral("再高一点"));
        QVERIFY(assistant.message().contains(QStringLiteral("温度边界")));
        QCOMPARE(vehicle.driverTemperature(), 30.0);
        QTest::qWait(950);
        nlu.submit(QStringLiteral("再低一度"));
        QCOMPARE(nlu.followUpOptions().size(), 2);
        QCOMPARE(vehicle.driverTemperature(), 30.0);
        nlu.endConversation();

        nlu.submit(QStringLiteral("主驾调到 24 度"));
        nlu.submit(QStringLiteral("现在还有多少电"));
        nlu.submit(QStringLiteral("再低一度"));
        QCOMPARE(nlu.followUpOptions().size(), 2);
        QCOMPARE(vehicle.driverTemperature(), 24.0);
        nlu.endConversation();
        nlu.submit(QStringLiteral("再开一点"));
        QCOMPARE(nlu.followUpOptions().size(), 4);
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 0.0);
    }
    void assistantReplaysClarificationsWithSimulator() {
        const auto recordings = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_DIALOGUE_WAVS");
        const auto simulatorPath = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_TEST_SIMULATOR");
        if (recordings.isEmpty() || simulatorPath.isEmpty())
            QSKIP("Supply dialogue WAV files and simulator executable");
        const auto server = "dialogue-test-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        QProcess simulator;
        simulator.start(simulatorPath, {"--headless", "--server", server});
        const auto stop = qScopeGuard([&] {
            if (simulator.state() != QProcess::NotRunning) {
                simulator.kill();
                simulator.waitForFinished();
            }
        });
        QVERIFY(simulator.waitForStarted());
        VehicleState vehicle;
        vehicle.connection()->setServerName(server);
        vehicle.setExternalMode(true);
        QTRY_VERIFY_WITH_TIMEOUT(vehicle.externalReady(), 5000);
        QLocalSocket operatorSocket;
        QByteArray controlBuffer;
        QString lastControl;
        connect(&operatorSocket, &QLocalSocket::readyRead, &operatorSocket, [&] {
            controlBuffer += operatorSocket.readAll();
            QList<QJsonObject> messages;
            QString error;
            if (!VehicleProtocol::decode(controlBuffer, messages, error))
                return;
            for (const auto &message : messages)
                if (message["type"] == "controlResult" && message["ok"].toBool())
                    lastControl = message["id"].toString();
        });
        operatorSocket.connectToServer(server);
        QTRY_COMPARE(operatorSocket.state(), QLocalSocket::ConnectedState);
        operatorSocket.write(VehicleProtocol::encode({{"type","hello"},{"role","operator"}}));
        auto control = [&](const QString &action, const QJsonObject &args) {
            const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            operatorSocket.write(VehicleProtocol::encode({{"type","control"},{"id",id},{"action",action},{"args",args}}));
            QTRY_COMPARE_WITH_TIMEOUT(lastControl, id, 2000);
        };
        AssistantVehicle controls(vehicle);
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos, [&](const QString &text, bool spoken, AssistantMedia::Reply reply) {
            return controls.submit(text, std::move(reply), spoken);
        });
        AssistantNlu nlu(vehicle, controls, assistant);
        AssistantSpeech speech(audio);
        QVERIFY(speech.configured());
        connect(&speech, &AssistantSpeech::recognized, &nlu, [&](const QString &text) { nlu.submit(text, true); });
        auto replay = [&](const QString &name) {
            speech.openWave(QUrl::fromLocalFile(recordings + '/' + name + ".wav"));
        };
        replay("temperature");
        QTRY_COMPARE_WITH_TIMEOUT(nlu.followUpOptions().size(), 2, 35000);
        QCOMPARE(vehicle.passengerTemperature(), 22.0);
        replay("passenger");
        QTRY_COMPARE_WITH_TIMEOUT(vehicle.passengerTemperature(), 24.0, 15000);
        QCOMPARE(vehicle.driverTemperature(), 22.0);
        replay("relative-temperature");
        QTRY_COMPARE_WITH_TIMEOUT(vehicle.passengerTemperature(), 23.0, 15000);
        QCOMPARE(vehicle.driverTemperature(), 22.0);

        replay("window");
        QTRY_COMPARE_WITH_TIMEOUT(nlu.followUpOptions().size(), 4, 35000);
        QCOMPARE(vehicle.windowTargets().at(3).toInt(), 0);
        replay("right-rear");
        QTRY_VERIFY_WITH_TIMEOUT(assistant.message().contains(QStringLiteral("右后车窗开度已调至 30%")), 18000);
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 30.0);
        replay("relative-window");
        QTRY_VERIFY_WITH_TIMEOUT(assistant.message().contains(QStringLiteral("右后车窗开度已调至 40%")), 18000);
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 40.0);
        replay("window");
        QTRY_COMPARE_WITH_TIMEOUT(nlu.followUpOptions().size(), 4, 35000);
        replay("cancel");
        QTRY_VERIFY_WITH_TIMEOUT(nlu.followUpOptions().isEmpty(), 15000);
        QCOMPARE(assistant.message(), QStringLiteral("已取消本次请求"));
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 40.0);

        control("faults", {{"delayMs", 500}});
        nlu.submit(QStringLiteral("右后窗开到 60%"));
        QTRY_VERIFY_WITH_TIMEOUT(controls.awaitingWindow(), 10000);
        nlu.submit(QStringLiteral("再开一点"));
        QVERIFY(assistant.message().contains(QStringLiteral("仍在执行")));
        QTRY_COMPARE_WITH_TIMEOUT(vehicle.windowActual().at(3).toDouble(), 60.0, 5000);
        QTRY_VERIFY(!assistant.pending());
        control("faults", {{"delayMs", 0}});
        control("window", {{"index", 3}, {"value", 95}});
        QTRY_COMPARE(vehicle.windowActual().at(3).toDouble(), 95.0);
        nlu.submit(QStringLiteral("再开一点"));
        QTRY_VERIFY_WITH_TIMEOUT(assistant.message().contains(QStringLiteral("右后车窗开度已调至 100%")), 5000);
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 100.0);
        nlu.submit(QStringLiteral("再开一点"));
        QVERIFY(assistant.message().contains(QStringLiteral("开度边界")));
        QCOMPARE(vehicle.connection()->pendingCount(), 0);

        control("faults", {{"reject", true}});
        nlu.submit(QStringLiteral("再关一点"));
        QTRY_VERIFY_WITH_TIMEOUT(assistant.message().contains(QStringLiteral("被拒绝")), 5000);
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 100.0);
        nlu.submit(QStringLiteral("再关一点"));
        QCOMPARE(nlu.followUpOptions().size(), 4);
        QVERIFY(!assistant.pending());
        nlu.cancelFollowUp();

        control("faults", {{"reject", false}});
        nlu.submit(QStringLiteral("右后窗开到 80%"));
        QTRY_VERIFY_WITH_TIMEOUT(assistant.message().contains(QStringLiteral("右后车窗开度已调至 80%")), 10000);
        simulator.kill();
        QVERIFY(simulator.waitForFinished());
        QTRY_VERIFY(!vehicle.externalReady());
        nlu.submit(QStringLiteral("再关一点"));
        QVERIFY(assistant.message().contains(QStringLiteral("未连接")));
        simulator.start(simulatorPath, {"--headless", "--server", server});
        QVERIFY(simulator.waitForStarted());
        QTRY_VERIFY_WITH_TIMEOUT(vehicle.externalReady(), 5000);
        nlu.submit(QStringLiteral("再关一点"));
        QCOMPARE(nlu.followUpOptions().size(), 4);
        QCOMPARE(vehicle.windowTargets().at(3).toInt(), 0);
    }
    void assistantRunsModelQueriesAndShadowsWrites() {
        if (!qEnvironmentVariableIsSet("CAR_TERMINAL_NLU_MODEL"))
            QSKIP("Supply rbtl3 runtime paths to exercise the model");
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto log = dir.filePath("corrections.jsonl");
        const auto previous = qgetenv("CAR_TERMINAL_NLU_LOG");
        const auto restore = qScopeGuard([&] {
            if (previous.isNull())
                qunsetenv("CAR_TERMINAL_NLU_LOG");
            else
                qputenv("CAR_TERMINAL_NLU_LOG", previous);
        });
        qputenv("CAR_TERMINAL_NLU_LOG", log.toUtf8());
        VehicleState vehicle;
        AssistantVehicle controls(vehicle);
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos, [&](const QString &text, bool spoken, AssistantMedia::Reply reply) {
            return controls.submit(text, std::move(reply), spoken);
        });
        AssistantNlu nlu(vehicle, controls, assistant);
        nlu.submit(QStringLiteral("SOC的实时值能显示一下吗"));
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 30000);
        QVERIFY2(nlu.diagnostic().contains(QStringLiteral("模型查询")), qPrintable(nlu.diagnostic()));
        QVERIFY(assistant.message().contains(QStringLiteral("82%")));
        nlu.submit(QStringLiteral("左前车窗开了多少"));
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 3000);
        QVERIFY(nlu.diagnostic().contains(QStringLiteral("模型查询")));
        QCOMPARE(assistant.message(), QStringLiteral("左前车窗已关闭"));
        vehicle.setAcOn(true);
        nlu.submit(QStringLiteral("别关空调"), true);
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 3000);
        QVERIFY(vehicle.acOn());
        QVERIFY(nlu.diagnostic().contains(QStringLiteral("AC_SET")));
        QVERIFY(nlu.diagnostic().contains(QStringLiteral("规则处理")));
        if (qEnvironmentVariableIsSet("CAR_TERMINAL_NLU_CANDIDATE")) {
            QTRY_VERIFY_WITH_TIMEOUT(nlu.review().value("candidateResult").toMap().contains("model"), 15000);
            QVERIFY(vehicle.acOn());
        }
        QVERIFY(!QFileInfo::exists(log));
        QVERIFY(!nlu.correct({}, "REJECT", "NONE", "[]", {}));
        QVERIFY(nlu.correct(QStringLiteral("别关空调"), "REJECT", "NONE", "{}", {}));
        QFile file(log);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto row = QJsonDocument::fromJson(file.readLine()).object();
        QCOMPARE(row.value("input").toString(), QStringLiteral("asr"));
        QCOMPARE(row.value("expected").toObject().value("speech_act").toString(), QStringLiteral("REJECT"));
        QCOMPARE(row.value("expected").toObject().value("intent").toString(), QStringLiteral("NONE"));
        QCOMPARE(row.value("original").toString(), QStringLiteral("别关空调"));
        QCOMPARE(row.value("modelResult").toObject().value("proposal").toObject().value("state").toBool(true), false);
        nlu.submit(QStringLiteral("看看空调滤芯寿命"));
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 3000);
        QVERIFY(!assistant.message().contains(QStringLiteral("82%")));
    }
    void assistantTunesFavoriteRadioAndWaitsForPlayback() {
        QTemporaryDir dir;
        const auto path = dir.filePath("broadcast.wav");
        QVERIFY(writeMusicFixture(path));
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        session.player()->setAudioOutput(nullptr);
        RadioService radio(&session);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos, {}, &radio);
        QSignalSpy played(&assistant, &AssistantMedia::viewRequested);
        assistant.submit(QStringLiteral("收听收藏电台中国之声"));
        QVERIFY(assistant.message().contains(QStringLiteral("没有找到")));
        QCOMPARE(radio.stationIdForName(QStringLiteral("经济之声"), false), QStringLiteral("cnr-2"));
        radio.toggleFavorite("cnr-1");
        radio.setQuery(QStringLiteral("经济"));
        const auto ticket = session.beginRadio("cnr-1", QStringLiteral("中国之声"));
        QVERIFY(ticket);
        assistant.submit(QStringLiteral("收听收藏电台中国之声"));
        QCOMPARE(assistant.message(), QStringLiteral("正在连接电台…"));
        QCOMPARE(played.size(), 0);
        session.openRadio(ticket, QUrl::fromLocalFile(path));
        QTRY_COMPARE_WITH_TIMEOUT(played.size(), 1, 5000);
        QCOMPARE(played.first().first().toString(), QStringLiteral("radio"));
        QCOMPARE(assistant.message(), QStringLiteral("正在收听 中国之声"));
    }
    void sourceGenerationAndMusicBookmark() {
        QTemporaryDir dir;
        const auto path = dir.path() + "/song.wav";
        QVERIFY(writeMusicFixture(path));
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        music.importTracks({QUrl::fromLocalFile(path)});
        music.restoreMusicPosition(QUrl::fromLocalFile(path).toString(), 4200);
        session.useMusic();
        QTRY_VERIFY_WITH_TIMEOUT(session.player()->duration() > 10000, 6000);
        QTRY_VERIFY_WITH_TIMEOUT(session.player()->position() >= 4000, 6000);
        const auto first = session.beginRadio("one", "One");
        QCOMPARE(qint64(music.position()), 4200);
        const auto second = session.beginRadio("two", "Two");
        QVERIFY(second > first);
        session.openRadio(first, QUrl::fromLocalFile(path));
        QVERIFY(session.player()->source().isEmpty());
        session.stop();
        session.openRadio(second, QUrl::fromLocalFile(path));
        QVERIFY(session.player()->source().isEmpty());
        session.useMusic();
        QTRY_VERIFY_WITH_TIMEOUT(session.player()->position() >= 4000, 6000);
        QVERIFY(!music.playing());
        music.setPlaying(true);
        QTRY_VERIFY_WITH_TIMEOUT(session.playing(), 6000);
        session.setPhoneActive(true);
        QVERIFY(!music.playing());
        QCOMPARE(session.kind(), "phone");
        music.setPlaying(true);
        QVERIFY(!music.playing());
        session.setPhoneActive(false);
        QCOMPARE(session.kind(), "music");
        QVERIFY(!session.playing());
    }
    void realVideoFramesSeekAndParking() {
        QTemporaryDir dir;
        const auto path = dir.path() + "/风景.avi";
        QVERIFY(writeVideoFixture(path));
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        QVideoSink sink;
        session.setVideoOutput(&sink);
        session.setVideoAllowed(false);
        session.playVideo(QUrl::fromLocalFile(path), "Landscape", 3000);
        QCOMPARE(session.phase(), "blocked");
        QVERIFY(!session.playing());
        session.setFullscreen(true);
        QVERIFY(!session.fullscreen());
        session.setVideoAllowed(true);
        QVERIFY(!session.playing());
        session.toggle();
        QTRY_VERIFY_WITH_TIMEOUT(session.player()->duration() > 17000, 6000);
        QTRY_VERIFY_WITH_TIMEOUT(sink.videoFrame().isValid(), 6000);
        QTRY_VERIFY_WITH_TIMEOUT(session.position() > 3000, 6000);
        session.seek(9000);
        QTRY_VERIFY_WITH_TIMEOUT(session.position() >= 8800, 4000);
        session.setRate(1.5);
        QCOMPARE(session.rate(), 1.5);
        session.setFullscreen(true);
        QVERIFY(session.fullscreen());
        session.setVideoAllowed(false);
        QVERIFY(!session.playing());
        QVERIFY(!session.fullscreen());
        QCOMPARE(session.phase(), "blocked");
        session.setVideoAllowed(true);
        QVERIFY(!session.playing());
        session.toggle();
        QTRY_VERIFY(session.playing());
        session.videoPageLeft();
        QVERIFY(!session.playing());
        const auto saved = session.snapshot();
        session.useMusic();
        session.restoreSnapshot(saved);
        QCOMPARE(session.kind(), "video");
        QTRY_VERIFY_WITH_TIMEOUT(session.position() > 8800, 6000);
        QVERIFY(!session.playing());
    }
    void videoLibraryPersistenceAndIndependentFiles() {
        QTemporaryDir dir;
        const auto root = dir.path() + "/videos", path = root + "/子目录/clip.avi",
                   store = dir.path() + "/profile/videos.json";
        QVERIFY(writeVideoFixture(path));
        const auto cache = dir.path() + "/profile/video-covers";
        QDir().mkpath(cache);
        const auto orphan = cache + "/" + QString(64, '0') + ".jpg", unmanaged = cache + "/notes.jpg";
        {
            QFile file(orphan);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("old");
        }
        {
            QFile file(unmanaged);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("keep");
        }
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        QVideoSink sink;
        session.setVideoOutput(&sink);
        {
            VideoLibrary library(&session, nullptr, store);
            library.importFolder(QUrl::fromLocalFile(root));
            QTRY_VERIFY_WITH_TIMEOUT(!library.scanning(), 10000);
            QCOMPARE(library.rowCount(), 1);
            QVERIFY(!QFileInfo::exists(orphan));
            QVERIFY(QFileInfo::exists(unmanaged));
            const auto row = library.index(0);
            QVERIFY(library.data(row, VideoLibrary::Duration).toLongLong() > 17000);
            QVERIFY(!library.data(row, VideoLibrary::Cover).toString().isEmpty());
            const auto url = library.data(row, VideoLibrary::Url).toString();
            library.play(url);
            QTRY_VERIFY_WITH_TIMEOUT(session.playing(), 6000);
            QTRY_VERIFY_WITH_TIMEOUT(session.canSeek(), 6000);
            session.seek(7000);
            QTRY_VERIFY_WITH_TIMEOUT(session.position() > 6900, 4000);
            session.pause();
            library.save();
            QVERIFY(QFileInfo::exists(path));
        }
        session.stop();
        VideoLibrary restored(&session, nullptr, store);
        QCOMPARE(restored.rowCount(), 1);
        QVERIFY(restored.data(restored.index(0), VideoLibrary::Bookmark).toLongLong() >= 6900);
        QVERIFY(!session.playing());
        restored.removeRoot(root);
        QCOMPARE(restored.rowCount(), 0);
        QVERIFY(QFileInfo::exists(path));
    }
    void rejectAudioOnlyVideo() {
        QTemporaryDir dir;
        const auto path = dir.path() + "/audio.avi";
        QVERIFY(writeMusicFixture(path));
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        session.playVideo(QUrl::fromLocalFile(path), "Audio only");
        QTRY_COMPARE_WITH_TIMEOUT(session.phase(), QString("error"), 6000);
        QVERIFY(!session.playing());
        QVERIFY(!session.error().isEmpty());
    }
    void libraryFiltersAndCancelledIntent() {
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        RadioService radio(&session);
        QCOMPARE(radio.rowCount(), 4);
        radio.setFilter("favorites");
        QCOMPARE(radio.rowCount(), 0);
        radio.toggleFavorite("cnr-1");
        QCOMPARE(radio.rowCount(), 1);
        radio.setQuery("not-a-station");
        QCOMPARE(radio.rowCount(), 0);
        radio.setQuery("");
        radio.setFilter("all");
        QCOMPARE(radio.rowCount(), 4);
        radio.tune("cnr-1");
        QVERIFY(session.busy());
        radio.stop();
        const auto saved = session.snapshot();
        session.useMusic();
        session.restoreSnapshot(saved);
        QCOMPARE(session.kind(), "radio");
        QCOMPARE(session.phase(), "stopped");
        QCOMPARE(session.itemId(), "cnr-1");
        QTest::qWait(200);
        QCOMPARE(session.phase(), "stopped");
        QVERIFY(session.player()->source().isEmpty());
        radio.toggleFavorite("cnr-2");
        radio.toggleFavorite("cnr-4");
        radio.tune("rthk-1");
        radio.next(-1);
        QCOMPARE(session.itemId(), "cnr-4");
        radio.stop();
    }
    void musicPreferenceMigrationAndRepeat() {
        QTemporaryDir dir;
        const auto old = dir.path() + "/old.ini", current = dir.path() + "/media.ini";
        {
            QSettings previous(old, QSettings::IniFormat);
            previous.setValue("volume", 63);
            previous.setValue("playMode", "one");
        }
        qunsetenv("CAR_TERMINAL_NO_SETTINGS");
        qputenv("CAR_TERMINAL_SETTINGS_FILE", old.toUtf8());
        qputenv("CAR_TERMINAL_MEDIA_SETTINGS_FILE", current.toUtf8());
        {
            MediaState music;
            QCOMPARE(music.volume(), 63);
            QCOMPARE(music.playMode(), "one");
            music.selectTrack(1);
            music.updateDuration(1000);
            QSignalSpy seeks(&music, &MediaState::seekRequested);
            music.seek(9999);
            QCOMPARE(seeks.last()[0].toLongLong(), 1000);
            music.finishTrack();
            QCOMPARE(music.track(), 1);
            music.setDemoActive(true);
            music.setVolume(5);
            music.setDemoActive(false);
            music.setVolume(61);
        }
        {
            MediaState restored;
            QCOMPARE(restored.volume(), 61);
        }
        qunsetenv("CAR_TERMINAL_SETTINGS_FILE");
        qunsetenv("CAR_TERMINAL_MEDIA_SETTINGS_FILE");
        qputenv("CAR_TERMINAL_NO_SETTINGS", "1");
    }
};
QTEST_MAIN(MediaSessionTests)
#include "MediaSessionTests.moc"

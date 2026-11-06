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
#include "assistant/AssistantVehicleQuery.h"
#include "assistant/AssistantNlu.h"
#include "assistant/AssistantAppearance.h"
#include "assistant/AssistantWake.h"
#include "assistant/AssistantVoice.h"
#include "VehicleState.h"
#include "protocol/VehicleProtocol.h"

class MediaSessionTests : public QObject {
    Q_OBJECT
  private slots:
    void initTestCase() { qputenv("CAR_TERMINAL_NO_SETTINGS", "1"); }
    void assistantImportsMp3AndSeparatesDictation() {
        const auto mp3 = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_SHORT_MP3");
        if (mp3.isEmpty())
            QSKIP("Supply a short MP3 fixture");
        AssistantAudioClip clip;
        QSignalSpy loaded(&clip, &AssistantAudioClip::loaded);
        clip.load(QUrl::fromLocalFile(mp3));
        QTest::qWait(1200);
        qInfo() << "MP3 decode" << clip.busy() << clip.duration() << clip.message();
        QTRY_VERIFY_WITH_TIMEOUT(!clip.busy(), 12000);
        QVERIFY2(clip.ready(), qPrintable(clip.message()));
        QCOMPARE(loaded.size(), 1);
        QVERIFY(clip.duration() > 3 && clip.duration() < 10);
        QVERIFY(clip.select(1, 2));
        QFile wave(clip.wave().toLocalFile());
        QVERIFY(wave.open(QIODevice::ReadOnly));
        QCOMPARE(wave.read(4), QByteArray("RIFF"));
        QCOMPARE(wave.size(), 64044LL);
        wave.close();
        clip.load(QUrl::fromLocalFile(mp3));
        clip.cancel();
        QTest::qWait(100);
        QVERIFY(!clip.ready());
        QVERIFY(!clip.busy());

        QAudioOutput output;
        AssistantSpeech speech(output);
        const auto words = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_CONTROL_WAVS") + "/driver-full.wav";
        QSignalSpy command(&speech, &AssistantSpeech::recognized);
        QSignalSpy dictation(&speech, &AssistantSpeech::dictated);
        speech.transcribe(QUrl::fromLocalFile(words), "preview");
        QTRY_COMPARE_WITH_TIMEOUT(dictation.size(), 1, 15000);
        QCOMPARE(command.size(), 0);
        QCOMPARE(dictation.first().at(1).toString(), QStringLiteral("preview"));
    }
    void assistantVoiceDraftAndPlaybackPolicy() {
        QTemporaryDir folder;
        AssistantVoice voice(folder.filePath("voice.json"));
        if (!voice.configured())
            QSKIP("Supply the TTS worker and model");
        const auto reference = qEnvironmentVariable("CAR_TERMINAL_TTS_REFERENCE");
        voice.setVolume(0);
        voice.importReference(QUrl::fromLocalFile(reference));
        QTRY_VERIFY_WITH_TIMEOUT(voice.dirty() && !voice.referenceClip()->busy(), 12000);
        voice.setTranscript(QStringLiteral("你好，我是你的座舱助手。"));
        voice.setPreviewText(QStringLiteral("欢迎出发。"));
        QCOMPARE(voice.transcript(), QStringLiteral("你好，我是你的座舱助手。"));
        QCOMPARE(voice.previewText(), QStringLiteral("欢迎出发。"));
        voice.preview();
        QTRY_VERIFY_WITH_TIMEOUT(!voice.busy(), 65000);
        QVERIFY2(voice.verified(), qPrintable(voice.message()));
        voice.stop();
        QVERIFY(voice.save(voice.transcript()));
        QVERIFY(!voice.dirty());
        voice.setTranscript(QStringLiteral("更改后的原文"));
        QVERIFY(!voice.verified());
        QVERIFY(!voice.save(voice.transcript()));
        voice.discardDraft();
        QCOMPARE(voice.transcript(), QStringLiteral("你好，我是你的座舱助手。"));
        QVERIFY(!voice.dirty());
        voice.setReplyMode("text");
        voice.reply(QStringLiteral("测试"), true);
        QVERIFY(!voice.busy());
        voice.setReplyMode("voice");
        voice.reply(QStringLiteral("测试"), false);
        QVERIFY(!voice.busy());
        voice.resetPlaybackPreferences();
        QCOMPARE(voice.volume(), 85);
        QCOMPARE(voice.speed(), 100);
        QCOMPARE(voice.replyMode(), QStringLiteral("all"));
        voice.recordReference();
        QCOMPARE(voice.countdown(), 3);
        voice.cancelRecording();
        QCOMPARE(voice.countdown(), 0);
        QVERIFY(!voice.referenceClip()->recording());
    }
    void assistantRelativeTargetsUseActualState() {
        VehicleState vehicle;
        AssistantVehicle controls(vehicle);
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos);
        AssistantNlu nlu(vehicle, controls, assistant);
        nlu.submit(QStringLiteral("主驾温度调高点"));
        QCOMPARE(vehicle.driverTemperature(), 22.5);
        QCOMPARE(vehicle.passengerTemperature(), 22.0);
        nlu.submit(QStringLiteral("副驾降两度"));
        QCOMPARE(vehicle.passengerTemperature(), 20.0);
        QCOMPARE(vehicle.driverTemperature(), 22.5);
        nlu.submit(QStringLiteral("再高一点"));
        QCOMPARE(vehicle.passengerTemperature(), 20.5);
        QCOMPARE(vehicle.driverTemperature(), 22.5);
        nlu.submit(QStringLiteral("不要把主驾温度调高点"));
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 10000);
        QCOMPARE(vehicle.driverTemperature(), 22.5);
        QVERIFY(!assistant.message().contains(QStringLiteral("可以点播本地媒体")));
        assistant.submit(QStringLiteral("有点热"));
        QVERIFY(assistant.message().contains(QStringLiteral("空调还是车窗")));
        QCOMPARE(vehicle.driverTemperature(), 22.5);
    }
    void assistantAbsoluteTemperatureAndQuietTransport() {
        QTemporaryDir directory;
        VehicleState vehicle;
        AssistantVehicle controls(vehicle);
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos);
        AssistantNlu nlu(vehicle, controls, assistant);
        nlu.submit(QStringLiteral("主驾温度调高到二十八度"), true);
        QCOMPARE(vehicle.driverTemperature(), 28.0);
        nlu.submit(QStringLiteral("副驾温度调低到二十三点五度"));
        QCOMPARE(vehicle.passengerTemperature(), 23.5);
        nlu.submit(QStringLiteral("不要把主驾温度调高到二十九度"), true);
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 10000);
        QCOMPARE(vehicle.driverTemperature(), 28.0);
        if (!session.outputAvailable()) QSKIP("Music test requires an output device");
        for (int i=0;i<2;++i) {
            const auto path=directory.filePath(QStringLiteral("track-%1.wav").arg(i));
            QVERIFY(writeMusicFixture(path));music.importTracks({QUrl::fromLocalFile(path)});
        }
        session.useMusic();music.selectTrack(0);music.setPlaying(true);
        QTRY_VERIFY_WITH_TIMEOUT(session.playing() && session.position()>0,6000);
        const auto before=music.trackUrl();
        AssistantVoice voice(directory.filePath("voice.json"));
        connect(&assistant,&AssistantMedia::replyReady,&voice,&AssistantVoice::reply);
        QSignalSpy replies(&assistant,&AssistantMedia::replyReady);
        nlu.submit(QStringLiteral("播放下一首"),true);
        QTRY_VERIFY_WITH_TIMEOUT(!assistant.pending(),6000);
        QVERIFY(music.trackUrl()!=before);QVERIFY(session.playing());
        QVERIFY(!replies.isEmpty());QCOMPARE(replies.last().at(2).toBool(),false);
        QVERIFY(!voice.busy());QVERIFY(!voice.playing());QVERIFY(!audio.isMuted());
        nlu.submit(QStringLiteral("暂停音乐"),true);
        QVERIFY(!session.playing());QCOMPARE(replies.last().at(2).toBool(),false);
    }
    void assistantEndsAutomaticCaptureOnPause() {
        QAudioFormat format;
        format.setSampleRate(16000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);
        const QByteArray silence(format.bytesForFrames(800), '\0');
        const QList<qint16> voiceSamples(800, 2800);
        const QByteArrayView voice(reinterpret_cast<const char *>(voiceSamples.constData()), format.bytesForFrames(800));
        AssistantEndpoint endpoint;
        for (int chunk = 0; chunk < 79; ++chunk)
            QCOMPARE(endpoint.observe(silence, format), AssistantEndpoint::Result::Listening);
        QCOMPARE(endpoint.observe(silence, format), AssistantEndpoint::Result::Silence);
        QVERIFY(!endpoint.spoke());

        endpoint = {};
        QCOMPARE(endpoint.observe(voice, format), AssistantEndpoint::Result::Listening);
        QCOMPARE(endpoint.observe(silence, format), AssistantEndpoint::Result::Listening);
        QVERIFY(!endpoint.spoke());
        QCOMPARE(endpoint.observe(voice, format), AssistantEndpoint::Result::Listening);
        QCOMPARE(endpoint.observe(voice, format), AssistantEndpoint::Result::Listening);
        QVERIFY(endpoint.spoke());
        for (int chunk = 0; chunk < 13; ++chunk)
            QCOMPARE(endpoint.observe(silence, format), AssistantEndpoint::Result::Listening);
        QCOMPARE(endpoint.observe(silence, format), AssistantEndpoint::Result::Finished);
    }
    void assistantCustomWakeReplaysWithoutListening() {
        AssistantWake wake;
        QVERIFY(!wake.enabled());
        QVERIFY(!wake.listening());
        QCOMPARE(wake.defaultKeyword(), QStringLiteral("你好周同学"));
        QCOMPARE(wake.keyword(), wake.defaultKeyword());
        const auto recordings = qEnvironmentVariable("CAR_TERMINAL_ASSISTANT_WAKE_WAVS");
        if (recordings.isEmpty() || !wake.configured())
            QSKIP("Supply KWS worker/model and offline wake samples");
        QSignalSpy activated(&wake, &AssistantWake::activated);
        auto replay = [&](const QString &sample) {
            wake.replay(QUrl::fromLocalFile(recordings + '/' + sample + ".wav"));
            QTRY_VERIFY_WITH_TIMEOUT(!wake.testing(), 12000);
        };
        replay("wake-default");
        QVERIFY(wake.testResult().startsWith(QStringLiteral("WAV 命中")));
        wake.saveKeyword(QStringLiteral("你好小林"));
        QTRY_VERIFY_WITH_TIMEOUT(!wake.testing(), 12000);
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
        wake.saveKeyword(wake.defaultKeyword());
        QTRY_VERIFY_WITH_TIMEOUT(!wake.testing(), 12000);
        QCOMPARE(wake.keyword(), QStringLiteral("你好周同学"));
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
        QSignalSpy replies(&assistant, &AssistantMedia::replyReady);
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
        QSignalSpy replies(&assistant, &AssistantMedia::replyReady);
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
    void assistantPromptTimeoutExcludesSpeech() {
        VehicleState vehicle;
        AssistantVehicle controls(vehicle);
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music,&audio,nullptr,true);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music,session,videos);
        AssistantNlu nlu(vehicle,controls,assistant,100);
        nlu.submit(QStringLiteral("空调调到24度"));
        QVERIFY(nlu.followUpActive());
        nlu.setPromptActive(true);
        QTest::qWait(220);
        QVERIFY(nlu.followUpActive());
        nlu.setPromptActive(false);
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.followUpActive(),1000);
    }
    void assistantClarificationWaitsAndExpires() {
        const auto original = qgetenv("CAR_TERMINAL_NLU_MODEL");
        QTemporaryDir missingModel;
        qputenv("CAR_TERMINAL_NLU_MODEL", missingModel.path().toUtf8());
        const auto restore = qScopeGuard([&] {
            if (!original.isNull())
                qputenv("CAR_TERMINAL_NLU_MODEL", original);
            else
                qunsetenv("CAR_TERMINAL_NLU_MODEL");
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

        nlu.submit(QStringLiteral("给我设个二十六度吧"));
        QCOMPARE(nlu.followUpOptions().size(), 2);
        nlu.submit(QStringLiteral("副驾"));
        QCOMPARE(vehicle.passengerTemperature(), 26.0);
        nlu.submit(QStringLiteral("副驾设成31度"));
        QVERIFY(nlu.followUpActive());
        QVERIFY(nlu.followUpOptions().isEmpty());
        nlu.submit(QStringLiteral("24.6度"));
        QVERIFY(assistant.message().contains(QStringLiteral("0.5")));
        QCOMPARE(vehicle.passengerTemperature(), 26.0);
        nlu.submit(QStringLiteral("二十四点五度"));
        QCOMPARE(vehicle.passengerTemperature(), 24.5);
        QVERIFY(!nlu.followUpActive());

        nlu.submit(QStringLiteral("给主驾设置23度并将风量调成4档"));
        QVERIFY(assistant.message().contains(QStringLiteral("多个调整")));
        QCOMPARE(vehicle.driverTemperature(), 24.0);
        nlu.submit(QStringLiteral("把左前关上，右后开到40%"));
        QVERIFY(assistant.message().contains(QStringLiteral("多个调整")));
        QCOMPARE(vehicle.windowTargets().at(3).toInt(), 0);

        nlu.submit(QStringLiteral("我问的是开到多少"));
        QCOMPARE(nlu.followUpOptions().size(), 4);
        nlu.submit(QStringLiteral("右后窗"));
        QVERIFY(assistant.message().contains(QStringLiteral("右后车窗已关闭")));
        QCOMPARE(vehicle.windowTargets().at(3).toInt(), 0);

        const int beforeFan = vehicle.fan();
        nlu.submit(QStringLiteral("麻烦风量调至五当"));
        QCOMPARE(vehicle.fan(), beforeFan);
        nlu.submit(QStringLiteral("麻烦风量调至五当"), true);
        QCOMPARE(vehicle.fan(), 5);
        nlu.submit(QStringLiteral("别把风量掉到三当"), true);
        QCOMPARE(vehicle.fan(), 5);
        nlu.submit(QStringLiteral("右后车窗降到三十五百分号"));
        QCOMPARE(vehicle.windowTargets().at(3).toInt(), 0);
        nlu.submit(QStringLiteral("现在剩余多少点"), true);
        QCOMPARE(nlu.followUpOptions(), QStringList{QStringLiteral("查询车辆电量")});
        QVERIFY(!assistant.message().contains(QStringLiteral("82%")));
        nlu.submit(QStringLiteral("确认"));
        QVERIFY(assistant.message().contains(QStringLiteral("82%")));

        nlu.submit(QStringLiteral("主驾调到23度"));
        nlu.submit(QStringLiteral("刚才说错了，改成副驾"));
        QCOMPARE(nlu.followUpOptions(), QStringList{QStringLiteral("确认调整")});
        vehicle.setClimateSync(true);
        vehicle.setDriverTemperature(25);
        nlu.submit(QStringLiteral("确认"));
        QVERIFY(assistant.message().contains(QStringLiteral("同步状态已变化")));
        QCOMPARE(vehicle.driverTemperature(), 25.0);
        QCOMPARE(vehicle.passengerTemperature(), 25.0);
        vehicle.setClimateSync(false);
        nlu.submit(QStringLiteral("副驾调到26度"));
        nlu.submit(QStringLiteral("温度不是这个，改一下"));
        QVERIFY(nlu.followUpActive());
        QVERIFY(assistant.message().contains(QStringLiteral("副驾改到多少度")));
        nlu.submit(QStringLiteral("二十五点五度"));
        QCOMPARE(vehicle.passengerTemperature(), 25.5);
        QCOMPARE(vehicle.driverTemperature(), 25.0);
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

        const QStringList approveBatch{QStringLiteral("确认执行全部")};
        const auto batch = QStringLiteral("先把右后窗开到42%，然后把主驾温度调到23度");
        nlu.submit(batch);
        QCOMPARE(nlu.followUpOptions(), approveBatch);
        QCOMPARE(vehicle.windowTargets().at(3).toInt(), 40);
        QCOMPARE(vehicle.driverTemperature(), 22.0);
        nlu.submit(QStringLiteral("确认执行"));
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.batchRunning(), 9000);
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 42.0);
        QCOMPARE(vehicle.driverTemperature(), 23.0);
        QCOMPARE(nlu.batchResults().size(), 2);
        for (const auto &row : nlu.batchResults())
            QCOMPARE(row.toMap().value("status").toString(), QStringLiteral("已确认"));

        control("faults", {{"reject", true}});
        nlu.submit(QStringLiteral("把左前车窗开到25%然后把主驾温度调到26度"));
        QCOMPARE(nlu.followUpOptions(), approveBatch);
        nlu.chooseFollowUp(0, nlu.followUpRevision());
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.batchRunning(), 9000);
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 0.0);
        QCOMPARE(vehicle.driverTemperature(), 23.0);
        QCOMPARE(nlu.batchResults().at(0).toMap().value("status").toString(), QStringLiteral("未确认"));
        QCOMPARE(nlu.batchResults().at(1).toMap().value("status").toString(), QStringLiteral("未执行"));
        control("faults", {{"reject", false}, {"delayMs", 500}});
        nlu.submit(QStringLiteral("把右后窗开到40%然后把主驾温度调到24度"));
        QCOMPARE(nlu.followUpOptions(), approveBatch);
        nlu.submit(QStringLiteral("确认"));
        nlu.cancelRemaining();
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.batchRunning(), 9000);
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 40.0);
        QCOMPARE(vehicle.driverTemperature(), 23.0);
        QCOMPARE(nlu.batchResults().at(1).toMap().value("status").toString(), QStringLiteral("未执行"));

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

        nlu.endConversation();
        replay("v7-fan");
        QTRY_COMPARE_WITH_TIMEOUT(vehicle.fan(), 5, 15000);
        replay("v7-battery");
        QTRY_COMPARE_WITH_TIMEOUT(nlu.followUpOptions(), QStringList{QStringLiteral("查询车辆电量")}, 15000);
        nlu.submit(QStringLiteral("确认"));
        QVERIFY(assistant.message().contains(QStringLiteral("82%")));
        replay("v7-window");
        QTRY_COMPARE_WITH_TIMEOUT(vehicle.windowActual().at(3).toDouble(), 35.0, 10000);
        QTRY_VERIFY(!assistant.pending());
        replay("v7-retarget");
        QTRY_COMPARE_WITH_TIMEOUT(nlu.followUpOptions(), QStringList{QStringLiteral("确认调整")}, 15000);
        QVERIFY(assistant.message().contains(QStringLiteral("右后窗当前开度 35%")));
        QCOMPARE(vehicle.windowTargets().at(0).toInt(), 0);
        const int revision = nlu.followUpRevision();
        nlu.submit(QStringLiteral("不确认"));
        nlu.chooseFollowUp(0, revision);
        QCOMPARE(vehicle.windowTargets().at(0).toInt(), 0);

        nlu.submit(QStringLiteral("右后窗开到35%"));
        QTRY_VERIFY_WITH_TIMEOUT(assistant.message().contains(QStringLiteral("右后车窗开度已调至 35%")), 10000);
        nlu.submit(QStringLiteral("不对，刚才那扇换成左前"));
        QCOMPARE(vehicle.windowTargets().at(0).toInt(), 0);
        replay("v7-confirm");
        QTRY_COMPARE_WITH_TIMEOUT(vehicle.windowActual().at(0).toDouble(), 35.0, 15000);
        QTRY_VERIFY(!assistant.pending());
        QCOMPARE(vehicle.windowActual().at(3).toDouble(), 35.0);

        if (qEnvironmentVariableIntValue("CAR_TERMINAL_NLU_WINDOW_PILOT") != 1)
            return;
        const auto auditStatus = [&] { return nlu.review().value("windowPilot").toMap().value("status").toString(); };
        const QStringList confirmation{QStringLiteral("确认调整")};
        const auto originalTargets = vehicle.windowTargets();
        const QStringList windowCommands{
            QStringLiteral("请帮我将左前车窗开度定到17%"),
            QStringLiteral("帮我把右前窗的开度调整至100%"),
            QStringLiteral("左后窗户的开度设到0%就好"),
            QStringLiteral("请将右后车窗的开度设为35%")};
        for (const auto &command : windowCommands) {
            nlu.submit(command);
            QTRY_COMPARE_WITH_TIMEOUT(nlu.followUpOptions(), confirmation, 10000);
            QCOMPARE(auditStatus(), QStringLiteral("awaiting_confirmation"));
            QCOMPARE(vehicle.windowTargets(), originalTargets);
            const auto pending = nlu.followUpRevision();
            nlu.submit(QStringLiteral("取消"));
            QCOMPARE(auditStatus(), QStringLiteral("cancelled"));
            nlu.chooseFollowUp(0, pending);
            QCOMPARE(vehicle.windowTargets(), originalTargets);
        }
        const QStringList nonCommands{
            QStringLiteral("别把右后窗的开度定到42%"),
            QStringLiteral("我只是在念把左前窗开度设到17%这句话"),
            QStringLiteral("不是让你把右前窗开度调整至87%"),
            QStringLiteral("把左前窗的开度调整至42%之前先问我"),
            QStringLiteral("把右后窗的开度设到42%再把左后窗关上"),
            QStringLiteral("右前和右后窗的开度都设到17%"),
            QStringLiteral("把右后窗的开度设到42%还是87%"),
            QStringLiteral("左前窗的开度设到-17%就好"),
            QStringLiteral("左前窗的开度调整至42.5%")};
        for (const auto &command : nonCommands) {
            nlu.submit(command);
            QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 10000);
            QVERIFY(nlu.followUpOptions() != confirmation);
            QCOMPARE(vehicle.windowTargets(), originalTargets);
            QCOMPARE(vehicle.connection()->pendingCount(), 0);
            nlu.endConversation();
        }
        nlu.submit(QStringLiteral("请把车窗开度调到17%"));
        QCOMPARE(nlu.followUpOptions().size(), 4);
        nlu.cancelFollowUp();
        nlu.submit(QStringLiteral("请把右前车窗开度调到120%"));
        QVERIFY(assistant.message().contains(QStringLiteral("100")));
        nlu.cancelFollowUp();
        QCOMPARE(vehicle.windowTargets(), originalTargets);

        const auto pilotWavs = qEnvironmentVariable("CAR_TERMINAL_NLU_WINDOW_WAVS");
        QVERIFY(!pilotWavs.isEmpty());
        speech.openWave(QUrl::fromLocalFile(pilotWavs + "/left-front.wav"));
        QTRY_COMPARE_WITH_TIMEOUT(nlu.followUpOptions(), confirmation, 15000);
        QCOMPARE(vehicle.windowTargets(), originalTargets);
        replay("v7-confirm");
        QTRY_COMPARE_WITH_TIMEOUT(auditStatus(), QStringLiteral("confirmed"), 15000);
        QCOMPARE(vehicle.windowActual().at(0).toDouble(), 17.0);
        for (int index = 1; index < 4; ++index)
            QCOMPARE(vehicle.windowTargets().at(index), originalTargets.at(index));

        operatorSocket.abort();
        operatorSocket.connectToServer(server);
        QTRY_COMPARE(operatorSocket.state(), QLocalSocket::ConnectedState);
        operatorSocket.write(VehicleProtocol::encode({{"type","hello"},{"role","operator"}}));
        control("faults", {{"reject", true}});
        nlu.submit(windowCommands.at(1));
        QTRY_COMPARE_WITH_TIMEOUT(nlu.followUpOptions(), confirmation, 10000);
        nlu.chooseFollowUp(0, nlu.followUpRevision());
        QTRY_COMPARE_WITH_TIMEOUT(auditStatus(), QStringLiteral("unconfirmed"), 10000);
        QVERIFY(assistant.message().contains(QStringLiteral("被拒绝")));
        QCOMPARE(vehicle.windowActual().at(1).toDouble(), 0.0);

        nlu.submit(windowCommands.at(1));
        QTRY_COMPARE_WITH_TIMEOUT(nlu.followUpOptions(), confirmation, 10000);
        const auto staleRevision = nlu.followUpRevision();
        simulator.kill();
        QVERIFY(simulator.waitForFinished());
        QTRY_VERIFY(!vehicle.externalReady());
        QTRY_COMPARE(auditStatus(), QStringLiteral("blocked"));
        nlu.chooseFollowUp(0, staleRevision);
        QCOMPARE(vehicle.connection()->pendingCount(), 0);
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
        QVERIFY(!row.value("modelResult").toObject().value("model").toString().isEmpty());
        nlu.submit(QStringLiteral("看看空调滤芯寿命"));
        QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 3000);
        QVERIFY(!assistant.message().contains(QStringLiteral("82%")));
    }
    void assistantReadOnlyQueryGuards() {
        VehicleState vehicle;
        const auto model = [](const QString &intent, const QJsonObject &proposal,
                              const QString &act = "QUERY", double confidence = 0.99) {
            return QJsonObject{{"speech_act", act}, {"intent", intent}, {"proposal", proposal}, {"confidence", confidence}};
        };
        const auto battery = model("BATTERY_QUERY", {});
        const auto front = model("WINDOW_QUERY", {{"target", "leftFront"}});
        QCOMPARE(assistantModelVehicleAnswer(vehicle, QStringLiteral("SOC的实时值能显示一下吗"), battery),
                 assistantBatteryAnswer(vehicle));
        QCOMPARE(assistantModelVehicleAnswer(vehicle, QStringLiteral("我想看看左前窗开度"), front),
                 assistantWindowAnswer(vehicle, 0));
        for (const auto &text : {QStringLiteral("看看空调滤芯寿命"), QStringLiteral("电池健康还有多少"),
                                 QStringLiteral("我手机还剩多少电"), QStringLiteral("别告诉我还剩多少电"),
                                 QStringLiteral("现在剩余多少点"), QStringLiteral("看看电量，然后关左前窗")})
            QVERIFY2(assistantModelVehicleAnswer(vehicle, text, battery).isEmpty(), qPrintable(text));
        for (const auto &text : {QStringLiteral("右后窗开度是多少"), QStringLiteral("左前和右后车窗开了多少"),
                                 QStringLiteral("别查左前窗开度"), QStringLiteral("左前窗擦得很亮"),
                                 QStringLiteral("把左前窗开到30%"), QStringLiteral("查一下车窗开度"),
                                 QStringLiteral("查左前窗开度，同时打开右前窗")})
            QVERIFY2(assistantModelVehicleAnswer(vehicle, text, front).isEmpty(), qPrintable(text));
        QVERIFY(assistantModelVehicleAnswer(vehicle, QStringLiteral("左前车窗开了多少"),
                model("WINDOW_QUERY", {{"target", "leftFront"}}, "EXECUTE")).isEmpty());
        QVERIFY(assistantModelVehicleAnswer(vehicle, QStringLiteral("左前车窗开了多少"),
                model("WINDOW_QUERY", {{"target", "leftFront"}}, "QUERY", 0.79)).isEmpty());
        QVERIFY(assistantModelVehicleAnswer(vehicle, QStringLiteral("左前车窗开了多少"),
                model("WINDOW_QUERY", {{"target", "leftFront"}, {"value", 80}})).isEmpty());
        vehicle.connection()->setServerName("readonly-offline-" + QUuid::createUuid().toString(QUuid::WithoutBraces));
        vehicle.setExternalMode(true);
        QVERIFY(assistantModelVehicleAnswer(vehicle, QStringLiteral("查一下车辆电量"), battery)
                .contains(QStringLiteral("不可用")));
        QVERIFY(assistantModelVehicleAnswer(vehicle, QStringLiteral("左前车窗开了多少"), front)
                .contains(QStringLiteral("不可用")));
    }
    void assistantReadOnlyQueryAcceptance() {
        const auto corpus = qEnvironmentVariable("CAR_TERMINAL_NLU_READONLY_CORPUS");
        const auto waves = qEnvironmentVariable("CAR_TERMINAL_NLU_READONLY_WAVS");
        if (corpus.isEmpty() || waves.isEmpty())
            QSKIP("Supply frozen read-only corpus and WAVs");
        VehicleState vehicle;
        for (int i = 0; i < 4; ++i) {
            vehicle.beginWindowDrag(i);
            vehicle.previewWindow(i, 15 * (i + 1));
            vehicle.endWindowDrag(i);
        }
        const auto openings = vehicle.windowActual();
        const auto initialTargets = vehicle.windowTargets();
        AssistantVehicle controls(vehicle);
        MediaState music;
        QAudioOutput audio;
        MediaSession session(&music, &audio, nullptr, true);
        VideoLibrary videos(&session);
        AssistantMedia assistant(music, session, videos, [&](const QString &text, bool spoken, AssistantMedia::Reply reply) {
            return controls.submit(text, std::move(reply), spoken);
        });
        AssistantNlu nlu(vehicle, controls, assistant);
        const auto battery = assistantBatteryAnswer(vehicle);
        const QStringList targets{"leftFront", "rightFront", "leftRear", "rightRear", "all"};
        int correct = 0, total = 0, modelAnswers = 0, batteryModel = 0, windowModel = 0;
        for (const auto *split : {"dev", "test", "challenge"}) {
            QFile file(corpus + '/' + split + ".jsonl");
            QVERIFY(file.open(QIODevice::ReadOnly));
            while (!file.atEnd()) {
                const auto row = QJsonDocument::fromJson(file.readLine()).object();
                const auto intent = row.value("intent").toString();
                if (intent != "BATTERY_QUERY" && intent != "WINDOW_QUERY")
                    continue;
                const auto text = row.value("text").toString();
                const auto target = row.value("expected_normalized").toObject().value("target").toString();
                const int index = int(targets.indexOf(target));
                QVERIFY(intent == "BATTERY_QUERY" || index >= 0);
                const auto expected = intent == "BATTERY_QUERY" ? battery : assistantWindowAnswer(vehicle, index == 4 ? -1 : index);
                nlu.submit(text);
                QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 10000);
                const bool viaModel = nlu.review().value("route").toString() == "model_query";
                const bool answered = assistant.message() == expected;
                ++total;
                correct += answered;
                modelAnswers += viaModel;
                batteryModel += viaModel && intent == "BATTERY_QUERY";
                windowModel += viaModel && intent == "WINDOW_QUERY";
                QVERIFY2(!viaModel || answered, qPrintable(text + " → " + assistant.message()));
                if (!answered)
                    qInfo().noquote() << "MISS" << text << assistant.message();
                QCOMPARE(vehicle.windowActual(), openings);
            }
        }
        QVERIFY(batteryModel > 0 && windowModel > 0);
        qInfo() << "READ_ONLY_TEXT" << correct << total << "model" << modelAnswers
                << "battery" << batteryModel << "window" << windowModel;
        const QStringList controlsToBlock{QStringLiteral("看看空调滤芯寿命"), QStringLiteral("左后车窗玻璃擦得很亮"),
            QStringLiteral("我手机还剩多少电"), QStringLiteral("我想知道电池健康还有百分之多少"),
            QStringLiteral("别告诉我左前窗开了多少"), QStringLiteral("能查一下左前和右后车窗的开度吗"),
            QStringLiteral("请查看车辆电量，然后关上左前车窗"), QStringLiteral("现在剩余多少点")};
        for (const auto &text : controlsToBlock) {
            nlu.submit(text);
            QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 10000);
            QVERIFY2(nlu.review().value("route").toString() != "model_query", qPrintable(text));
            QVERIFY2(!assistant.message().startsWith(QStringLiteral("当前电量")) &&
                     !assistant.message().contains(QStringLiteral("车窗开度 ")), qPrintable(text));
            QCOMPARE(vehicle.windowActual(), openings);
            QCOMPARE(vehicle.windowTargets(), initialTargets);
            nlu.endConversation();
        }
        qInfo() << "READ_ONLY_NEGATIVES" << controlsToBlock.size();

        AssistantSpeech speech(audio);
        QVERIFY(speech.configured());
        QSignalSpy recognized(&speech, &AssistantSpeech::recognized);
        connect(&speech, &AssistantSpeech::recognized, &nlu, [&](const QString &text) { nlu.submit(text, true); });
        const QList<QPair<QString, QString>> queries{{"battery", battery}, {"left-front", assistantWindowAnswer(vehicle, 0)},
            {"right-rear", assistantWindowAnswer(vehicle, 3)}, {"negative", {}}, {"unclear", {}}};
        int audioCorrect = 0;
        int audioTotal = 0;
        for (const auto &[file, expected] : queries) {
            speech.openWave(QUrl::fromLocalFile(waves + '/' + file + ".wav"));
            ++audioTotal;
            QTRY_COMPARE_WITH_TIMEOUT(recognized.size(), audioTotal, 15000);
            QTRY_VERIFY_WITH_TIMEOUT(!nlu.busy(), 10000);
            const auto answer = assistant.message();
            const bool ok = expected.isEmpty() ? answer != battery && !answer.contains(QStringLiteral("车窗开度"))
                                               : answer == expected;
            audioCorrect += ok;
            if (expected.isEmpty())
                QVERIFY2(ok, qPrintable(recognized.last().at(0).toString() + " → " + answer));
            qInfo().noquote() << "READ_ONLY_WAV" << file << ok << recognized.last().at(0).toString() << answer;
            nlu.endConversation();
            QCOMPARE(vehicle.windowActual(), openings);
        }
        qInfo() << "READ_ONLY_WAV_SUMMARY" << audioCorrect << audioTotal;
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
            const auto first = dir.filePath("first.wav"), second = dir.filePath("second.wav");
            QVERIFY(writeMusicFixture(first));
            QVERIFY(writeMusicFixture(second));
            music.importTracks({QUrl::fromLocalFile(first), QUrl::fromLocalFile(second)});
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

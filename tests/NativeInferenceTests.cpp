#include "assistant/inference/BertTokenizer.h"
#include "assistant/inference/InferenceEngine.h"
#include "assistant/inference/NativeAudio.h"
#include "assistant/inference/NluSlots.h"
#include "assistant/inference/VoiceText.h"
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>

class NativeInferenceTests : public QObject {
    Q_OBJECT
  private slots:
    void numericSlotGrounding() {
        auto temperature = [](const QString &literal) {
            const auto text = QStringLiteral("主驾温度调到") + literal + QStringLiteral("度");
            const QJsonArray entities{
                QJsonObject{{"label", "ZONE"}, {"text", "主驾"}, {"start", 0}, {"end", 2}},
                QJsonObject{{"label", "VALUE"}, {"text", literal}, {"start", 6}, {"end", 6 + literal.size()}},
                QJsonObject{{"label", "UNIT"},
                            {"text", "度"},
                            {"start", 6 + literal.size()},
                            {"end", 7 + literal.size()}}};
            return cockpit::normalizeSlots(text, "text", "EXECUTE", "TEMPERATURE_SET", entities);
        };
        QCOMPARE(temperature(QStringLiteral("二十八"))["value"].toDouble(), 28.0);
        QCOMPARE(temperature(QStringLiteral("22.5"))["value"].toDouble(), 22.5);
        for (const auto &invalid : {QStringLiteral("+28"), QStringLiteral("-28"), QStringLiteral("负二十八"),
                                    QStringLiteral("28%"), QStringLiteral("二十八点"), QStringLiteral("2e1")})
            QVERIFY_EXCEPTION_THROWN(temperature(invalid), std::runtime_error);
    }
    void modelBundleIntegrity() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        auto write = [&](const QString &name, const QByteArray &bytes) {
            QFile file(directory.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(bytes), bytes.size());
        };
        const QByteArray content("fixture model data");
        write("model.onnx", content);
        const auto hash =
            QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex());
        write("bundle.json", QJsonDocument(QJsonObject{{"schema", "cockpit-native-bundle-v1"},
                                                       {"files", QJsonObject{{"model.onnx", hash}}}})
                                 .toJson());
        cockpit::verifyBundle(directory.path());
        write("model.onnx", "changed model data");
        QVERIFY_EXCEPTION_THROWN(cockpit::verifyBundle(directory.path()), std::runtime_error);
        write("bundle.json", QJsonDocument(QJsonObject{{"schema", "cockpit-native-bundle-v1"},
                                                       {"files", QJsonObject{{"../outside.onnx", hash}}}})
                                 .toJson());
        QVERIFY_EXCEPTION_THROWN(cockpit::verifyBundle(directory.path()), std::runtime_error);
    }
    void tokenizerAndNluParity() {
        const auto fixture = qEnvironmentVariable("COCKPIT_NLU_PARITY");
        if (fixture.isEmpty())
            QSKIP("Supply exported NLU parity fixtures");
        QFile file(fixture);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto rows = QJsonDocument::fromJson(file.readAll()).array();
        QVERIFY(!rows.isEmpty());
        const auto path = cockpit::modelDirectory(cockpit::InferenceKind::Nlu);
        cockpit::BertTokenizer tokenizer(path + "/vocab.txt");
        auto engine = cockpit::makeNlu(path);
        std::atomic_bool cancelled = false;
        int tokenErrors = 0, headErrors = 0, proposalErrors = 0, entityErrors = 0;
        QJsonArray failures;
        QElapsedTimer timer;
        timer.start();
        for (const auto &entry : rows) {
            const auto row = entry.toObject();
            const auto text = row["text"].toString();
            const auto tokens = tokenizer.encode(text);
            QJsonArray ids, offsets;
            for (const auto &token : tokens) {
                ids.append(qint64(token.id));
                offsets.append(QJsonArray{token.start, token.end});
            }
            const auto actual = engine->run({{"text", text}, {"input", row["input"]}}, cancelled);
            const auto expected = row["expected"].toObject();
            const bool tokenOk = ids == row["ids"].toArray() && offsets == row["offsets"].toArray();
            const bool headOk =
                actual["intent"] == expected["intent"] && actual["speech_act"] == expected["speech_act"];
            const bool proposalOk = actual["proposal"] == expected["proposal"];
            const bool entityOk = actual["entities"] == expected["entities"];
            tokenErrors += !tokenOk;
            headErrors += !headOk;
            proposalErrors += !proposalOk;
            entityErrors += !entityOk;
            if (!tokenOk || !headOk || !proposalOk || !entityOk)
                failures.append(QJsonObject{
                    {"text", text}, {"tokenOk", tokenOk}, {"actual", actual}, {"expected", expected}});
        }
        qInfo() << "NATIVE_PARITY" << rows.size() << "token" << tokenErrors << "head" << headErrors
                << "proposal" << proposalErrors << "entity" << entityErrors << "ms" << timer.elapsed();
        const auto report = qEnvironmentVariable("COCKPIT_NATIVE_PARITY_REPORT");
        if (!report.isEmpty()) {
            QFile out(report);
            QVERIFY(out.open(QIODevice::WriteOnly));
            out.write(QJsonDocument(failures).toJson());
        }
        QCOMPARE(tokenErrors, 0);
        QCOMPARE(headErrors, 0);
        QCOMPARE(proposalErrors, 0);
        QCOMPARE(entityErrors, 0);
    }
    void residentAsr() {
        const auto wav = qEnvironmentVariable("COCKPIT_NATIVE_ASR_WAV");
        if (wav.isEmpty())
            QSKIP("Supply ASR sample");
        QFile file(wav);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QElapsedTimer loadTimer;
        loadTimer.start();
        auto engine = cockpit::makeAsr(cockpit::modelDirectory(cockpit::InferenceKind::Asr));
        const auto loadMs = loadTimer.elapsed();
        std::atomic_bool cancelled = false;
        const QJsonObject request{{"format", QJsonArray{"wav"}},
                                  {"audio", QString::fromLatin1(file.readAll().toBase64())}};
        QElapsedTimer timer;
        timer.start();
        const auto first = engine->run(request, cancelled);
        const auto firstMs = timer.restart();
        const auto second = engine->run(request, cancelled);
        qInfo() << "NATIVE_ASR" << first << "load ms" << loadMs << "first ms" << firstMs << "warm ms"
                << timer.elapsed();
        QVERIFY(!first["text"].toString().isEmpty());
        QCOMPARE(first["text"], second["text"]);
    }
    void completeLongVoice() {
        const auto reference = cockpit::modelResource("defaultReference");
        if (reference.isEmpty())
            QSKIP("Supply native model manifest");
        QFile transcript(cockpit::modelResource("defaultTranscript"));
        QVERIFY(transcript.open(QIODevice::ReadOnly));
        QTemporaryDir temporary;
        const auto output = temporary.filePath("long.wav");
        auto voice = cockpit::makeVoice(cockpit::modelDirectory(cockpit::InferenceKind::Voice));
        std::atomic_bool cancelled = false;
        const QJsonObject request{
            {"text", QStringLiteral(
                         "车窗已关闭。主驾温度已调到二十八度。现在播放下一首音乐。最后一句，祝你一路顺风。")},
            {"reference", reference},
            {"transcript", QString::fromUtf8(transcript.readAll()).trimmed()},
            {"output", output}};
        QElapsedTimer timer;
        timer.start();
        const auto result = voice->run(request, cancelled);
        qInfo() << "NATIVE_LONG_TTS" << result << "ms" << timer.elapsed();
        QVERIFY(result["segments"].toInt() >= 5);
        auto wave = cockpit::readWave(output, 120);
        QVERIFY(!wave.samples.empty());
        QCOMPARE(result["seconds"].toDouble(), double(wave.samples.size()) / wave.rate);
        const auto first =
            wave.samples.size() > size_t(wave.rate * 5) ? wave.samples.size() - wave.rate * 5 : 0;
        const QByteArray tail(reinterpret_cast<const char *>(wave.samples.data() + first),
                              (wave.samples.size() - first) * sizeof(float));
        auto asr = cockpit::makeAsr(cockpit::modelDirectory(cockpit::InferenceKind::Asr));
        const auto recognized = asr->run({{"audio", QString::fromLatin1(tail.toBase64())},
                                          {"format", QJsonArray{"f32", QString::number(wave.rate), "1"}}},
                                         cancelled);
        qInfo() << "NATIVE_LONG_TTS_TAIL" << recognized;
        QVERIFY(recognized["text"].toString().contains(QStringLiteral("一路")));
    }
    void voiceTextReadsNumericValues() {
        QCOMPARE(cockpit::voiceTextForSynthesis(QStringLiteral("主驾空调已设为 28 度")),
                 QStringLiteral("主驾空调已设为 二十八 度"));
        QCOMPARE(cockpit::voiceTextForSynthesis(QStringLiteral("温度22.5度，开度35%，电量100％")),
                 QStringLiteral("温度二十二点五度，开度百分之三十五，电量百分之一百"));
        QCOMPARE(cockpit::voiceTextForSynthesis(QStringLiteral("-3.05度，风量0挡，10秒，余量102")),
                 QStringLiteral("负三点零五度，风量零挡，十秒，余量一百零二"));
        QCOMPARE(cockpit::voiceTextForSynthesis(QStringLiteral("我在。")), QStringLiteral("我在。"));
        QCOMPARE(cockpit::voiceTextForSynthesis(QStringLiteral("要设为28度吗？")), QStringLiteral("要设为二十八度吗？"));
    }
    void spokenTemperatureIncludesValue() {
        const auto reference = cockpit::modelResource("defaultReference");
        if (reference.isEmpty())
            QSKIP("Supply native model manifest");
        QFile transcript(cockpit::modelResource("defaultTranscript"));
        QVERIFY(transcript.open(QIODevice::ReadOnly));
        QTemporaryDir temporary;
        const auto output = temporary.filePath("temperature.wav");
        auto voice = cockpit::makeVoice(cockpit::modelDirectory(cockpit::InferenceKind::Voice));
        std::atomic_bool cancelled = false;
        const auto result = voice->run({{"text", QStringLiteral("主驾空调已设为 28 度")},
                                        {"reference", reference},
                                        {"transcript", QString::fromUtf8(transcript.readAll()).trimmed()},
                                        {"output", output}}, cancelled);
        QVERIFY(!result.contains("error"));
        const auto audio = cockpit::readWave(output);
        const auto silenceSamples = size_t(audio.rate * 180 / 1000);
        QVERIFY(audio.samples.size() > silenceSamples);
        QVERIFY(std::all_of(audio.samples.end() - silenceSamples, audio.samples.end(),
                            [](float sample) { return sample == 0; }));
        QFile wave(output);
        QVERIFY(wave.open(QIODevice::ReadOnly));
        auto asr = cockpit::makeAsr(cockpit::modelDirectory(cockpit::InferenceKind::Asr));
        const auto recognized = asr->run({{"audio", QString::fromLatin1(wave.readAll().toBase64())},
                                           {"format", QJsonArray{"wav"}}}, cancelled);
        const auto text = recognized["text"].toString().remove(QChar(u' '));
        qInfo() << "NATIVE_TEMPERATURE_TTS" << result << recognized;
        QVERIFY2(text.contains(QStringLiteral("28度")) || text.contains(QStringLiteral("二十八度")), qPrintable(text));
    }
};
QTEST_GUILESS_MAIN(NativeInferenceTests)
#include "NativeInferenceTests.moc"

#include "AssistantNlu.h"
#include "AssistantMedia.h"
#include "AssistantVehicle.h"
#include "AssistantVehicleQuery.h"
#include "VehicleState.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QPointer>
#include <QRegularExpression>
#include <QStringList>

AssistantNlu::AssistantNlu(VehicleState &state, AssistantVehicle &vehicle, AssistantMedia &media, int followUpTimeoutMs)
    : m_state(state), m_vehicle(vehicle), m_media(media),
      m_worker(qEnvironmentVariable("CAR_TERMINAL_NLU_PYTHON"), qEnvironmentVariable("CAR_TERMINAL_NLU_WORKER"),
               qEnvironmentVariable("CAR_TERMINAL_NLU_MODEL"), 25000),
      m_candidate(qEnvironmentVariable("CAR_TERMINAL_NLU_PYTHON"), qEnvironmentVariable("CAR_TERMINAL_NLU_WORKER"),
                  qEnvironmentVariable("CAR_TERMINAL_NLU_CANDIDATE"), 25000) {
    m_diagnostic = m_worker.configured() ? QStringLiteral("rbtl3 · 只读查询与车控对照") : QStringLiteral("规则模式");
    connect(&m_worker, &AssistantWorker::completed, this, &AssistantNlu::complete);
    connect(&m_candidate, &AssistantWorker::completed, this, [this](const QJsonObject &result) {
        if (m_candidateRevision != m_reviewRevision) {
            submitCandidate();
            return;
        }
        m_candidatePending = false;
        m_last.insert("candidateResult", result);
        recordReview();
        emit changed();
    });
    m_followUpTimeout.setSingleShot(true);
    m_followUpTimeout.setInterval(followUpTimeoutMs);
    m_contextTimeout.setSingleShot(true);
    m_contextTimeout.setInterval(followUpTimeoutMs);
    connect(&m_contextTimeout, &QTimer::timeout, this, &AssistantNlu::clearContext);
    connect(&m_followUpTimeout, &QTimer::timeout, this, [this] {
        clearFollowUp();
        m_media.answer(QStringLiteral("等待超时，请重新说出完整指令"), false);
        emit changed();
    });
    connect(&state, &VehicleState::sourceChanged, this, [this] {
        if (m_lastSetting && !sourceMatches(m_contextSource, m_contextSession))
            clearContext();
        if (m_followUp && !sourceMatches(m_followUpSource, m_followUpSession)) {
            clearFollowUp();
            m_media.answer(QStringLiteral("车辆数据来源已变化，请重新发出指令"), false);
            emit changed();
        }
    });
}

void AssistantNlu::submit(const QString &text, bool spoken) {
    const auto request = text.trimmed();
    if (request.isEmpty() || request.size() > 256)
        return;
    if (assistantCancelsRequest(request)) {
        beginReview(request, spoken);
        if (m_followUp || m_active || !m_media.choices().isEmpty()) {
            endConversation();
            m_media.answer(QStringLiteral("已取消本次请求"), spoken);
        } else if (m_media.pending()) {
            clearContext();
            m_media.vehicleReply()(QStringLiteral("指令已提交，等待执行结果"), false);
        } else {
            clearContext();
            m_media.answer(QStringLiteral("已结束本次指令"), spoken);
        }
        finishReview(QStringLiteral("cancel"));
        emit changed();
        return;
    }
    if (m_followUp) {
        const int target = m_followUp->targetFromReply(request);
        if (target >= 0) {
            const auto previous = m_last.value("text").toString();
            beginReview(request, spoken, {{"previousText", previous}, {"result", "awaiting_target"},
                {"kind", m_followUp->kind == AssistantVehicleSetting::Kind::Window ? "window" : "temperature"}});
            finishFollowUp(target, spoken);
            finishReview(QStringLiteral("target_reply"));
            emit changed();
            return;
        }
        clearFollowUp();
    }
    if (auto relative = assistantRelativeVehicleSetting(request)) {
        if (m_vehicle.awaitingWindow() || (busy() && assistantVehicleSetting(m_text, m_spoken))) {
            m_media.vehicleReply()(m_vehicle.awaitingWindow()
                ? QStringLiteral("上一条车窗指令仍在执行，请等待结果")
                : QStringLiteral("正在处理上一条车控指令，请等待结果"), false);
            return;
        }
        QJsonObject context;
        if (m_lastSetting && m_lastSetting->kind == relative->kind) {
            relative->target = m_lastSetting->target;
            context = {{"previousText", m_contextText}, {"result", "confirmed"},
                       {"target", m_lastSetting->targets().at(relative->target)}, {"confirmedValue", m_contextValue}};
        }
        clearContext();
        m_superseded = m_active;
        beginReview(request, spoken, context);
        m_media.beginRequest(spoken);
        const bool clarifying = dispatchSetting(*relative, spoken);
        m_diagnostic = clarifying ? QStringLiteral("连续调整 · 等待补全") : QStringLiteral("连续调整 · 规则执行");
        finishReview(clarifying ? QStringLiteral("clarify") : QStringLiteral("relative_rules"));
        emit changed();
        return;
    }
    clearContext();
    if (!m_active && m_media.chooseByRequest(request, spoken))
        return;
    m_vehicle.reset();
    m_media.beginRequest(spoken);
    beginReview(request, spoken);
    if (m_active || !m_worker.submit({{"text", request}})) {
        m_superseded = m_active;
        const bool clarifying = dispatchRules(request, spoken);
        m_diagnostic = clarifying ? QStringLiteral("规则模式 · 等待补全") : QStringLiteral("规则模式");
        finishReview(clarifying ? QStringLiteral("clarify") : QStringLiteral("rules"));
    } else {
        m_text = request;
        m_spoken = spoken;
        m_active = true;
        m_superseded = false;
        m_diagnostic = QStringLiteral("rbtl3 正在分析…");
    }
    emit changed();
}

QStringList AssistantNlu::followUpOptions() const {
    return m_followUp ? m_followUp->targets() : QStringList();
}

bool AssistantNlu::dispatchRules(const QString &request, bool spoken) {
    if (const auto setting = assistantVehicleSetting(request, spoken))
        return dispatchSetting(*setting, spoken);
    m_media.submit(request, spoken);
    return false;
}

bool AssistantNlu::dispatchSetting(const AssistantVehicleSetting &setting, bool spoken) {
    if (const auto error = setting.rangeError(); !error.isEmpty()) {
        m_media.answer(error, spoken);
        return false;
    }
    if (setting.target >= 0) {
        executeSetting(setting);
        return false;
    }
    if (setting.kind == AssistantVehicleSetting::Kind::Window && !m_state.externalReady()) {
        m_media.answer(QStringLiteral("模拟器未连接，请恢复联调后重新发出指令"), spoken);
        return false;
    }
    m_followUp = setting;
    m_followUpSource = m_state.sourceMode();
    m_followUpSession = m_state.connection()->sessionId();
    m_followUpSpoken = spoken;
    ++m_followUpRevision;
    m_followUpTimeout.start();
    m_media.answer(setting.prompt(), spoken);
    return true;
}

void AssistantNlu::executeSetting(const AssistantVehicleSetting &setting) {
    const auto source = m_state.sourceMode();
    const auto session = m_state.connection()->sessionId();
    m_vehicle.execute(setting, m_media.vehicleReply(), [self = QPointer<AssistantNlu>(this),
        revision = m_contextRevision, source, session, setting, text = m_last.value("text").toString()] {
        if (!self || revision != self->m_contextRevision || !self->sourceMatches(source, session))
            return;
        self->m_lastSetting = setting;
        self->m_contextSource = source;
        self->m_contextSession = session;
        self->m_contextText = text;
        self->m_contextValue = setting.kind == AssistantVehicleSetting::Kind::Window
            ? self->m_state.windowActual().at(setting.target).toDouble()
            : setting.target == 1 ? self->m_state.passengerTemperature() : self->m_state.driverTemperature();
        self->m_contextTimeout.start();
    });
}

void AssistantNlu::clearContext() {
    ++m_contextRevision;
    m_lastSetting.reset();
    m_contextTimeout.stop();
}

bool AssistantNlu::sourceMatches(const QString &source, const QString &session) const {
    return source == m_state.sourceMode() && m_state.externalReady() && session == m_state.connection()->sessionId();
}

void AssistantNlu::chooseFollowUp(int index, int revision) {
    if (m_followUp && revision == m_followUpRevision)
        finishFollowUp(index, m_followUpSpoken);
}

void AssistantNlu::finishFollowUp(int target, bool spoken) {
    if (!m_followUp || target < 0 || target >= m_followUp->targets().size())
        return;
    auto setting = *m_followUp;
    setting.target = target;
    clearFollowUp();
    m_media.beginRequest(spoken);
    executeSetting(setting);
    m_diagnostic = QStringLiteral("已补全目标 · 规则执行");
    emit changed();
}

void AssistantNlu::clearFollowUp() {
    if (!m_followUp)
        return;
    m_followUp.reset();
    ++m_followUpRevision;
    m_followUpTimeout.stop();
}

void AssistantNlu::cancelFollowUp() {
    if (!m_followUp)
        return;
    clearFollowUp();
    m_media.answer(QStringLiteral("已取消本次请求"), false);
    emit changed();
}

void AssistantNlu::endConversation() {
    clearFollowUp();
    clearContext();
    m_superseded = m_active;
    emit changed();
}

void AssistantNlu::complete(const QJsonObject &result) {
    m_active = false;
    if (m_superseded) {
        emit changed();
        return;
    }
    const auto intent = result.value("intent").toString();
    const auto act = result.value("speech_act").toString();
    const auto proposal = result.value("proposal");
    QString answer;
    if (act == "QUERY" && proposal.isObject() && result.value("confidence").toDouble() >= 0.80 &&
        !assistantVehicleSetting(m_text, m_spoken)) {
        static const QRegularExpression batteryTopic(QStringLiteral("电量|电池|剩.{0,5}电|还有.{0,5}电|\\bsoc\\b"),
                                                     QRegularExpression::CaseInsensitiveOption);
        if (intent == "BATTERY_QUERY" && proposal.toObject().isEmpty() && batteryTopic.match(m_text).hasMatch())
            answer = assistantBatteryAnswer(m_state);
        if (intent == "WINDOW_QUERY" && m_text.contains(QChar(u'窗'))) {
            static const QStringList targets{"leftFront", "rightFront", "leftRear", "rightRear", "all"};
            const auto index = targets.indexOf(proposal.toObject().value("target").toString());
            if (index >= 0)
                answer = assistantWindowAnswer(m_state, index == 4 ? -1 : int(index));
        }
        const auto ruleAnswer = assistantVehicleAnswer(m_state, m_text);
        if (!ruleAnswer.isEmpty() && ruleAnswer != answer)
            answer.clear();
    }
    m_last.insert("modelResult", result);
    bool clarifying = false;
    if (answer.isEmpty())
        clarifying = dispatchRules(m_text, m_spoken);
    else
        m_media.answer(answer, m_spoken);
    m_diagnostic = intent.isEmpty() ? QStringLiteral("rbtl3 未给出候选 · 规则处理")
        : QStringLiteral("rbtl3 %1 · %2 · %3").arg(act, intent, clarifying ? QStringLiteral("等待补全")
            : answer.isEmpty() ? QStringLiteral("规则处理") : QStringLiteral("模型查询"));
    if (proposal.isObject() && !proposal.toObject().isEmpty())
        m_diagnostic += QStringLiteral(" · ") + QString::fromUtf8(QJsonDocument(proposal.toObject()).toJson(QJsonDocument::Compact));
    finishReview(clarifying ? QStringLiteral("clarify")
                           : answer.isEmpty() ? QStringLiteral("rules") : QStringLiteral("model_query"));
    emit changed();
}

void AssistantNlu::beginReview(const QString &request, bool spoken, const QJsonObject &context) {
    m_candidatePending = false;
    recordReview();
    ++m_reviewRevision;
    m_reviewSaved = false;
    m_last = {{"schema", 2}, {"text", request}, {"input", spoken ? "asr" : "text"}};
    if (!context.isEmpty())
        m_last.insert("context", context);
    m_candidatePending = m_candidate.configured();
    submitCandidate();
}

void AssistantNlu::submitCandidate() {
    if (m_candidatePending && m_candidateRevision != m_reviewRevision &&
        m_candidate.submit({{"text", m_last.value("text")}}))
        m_candidateRevision = m_reviewRevision;
}

void AssistantNlu::finishReview(const QString &route) {
    m_last.insert("route", route);
    m_last.insert("response", m_media.message());
    recordReview();
}

void AssistantNlu::recordReview() {
    if (!m_recordingSamples || !canCorrect() || m_candidatePending || m_reviewSaved)
        return;
    m_reviewSaved = true;
    if (!append(m_last))
        m_diagnostic += QStringLiteral(" · 样本保存失败");
}

bool AssistantNlu::correct(const QString &original, const QString &act, const QString &intent,
                           const QString &parametersJson, const QString &trainingText) {
    static const QStringList acts{"EXECUTE", "QUERY", "CANCEL", "CLARIFY", "REJECT"};
    static const QStringList intents{"NONE", "WINDOW_SET", "WINDOW_QUERY", "TEMPERATURE_SET", "AC_SET",
        "FAN_SET", "BATTERY_QUERY", "MEDIA_CONTROL", "RADIO_TUNE", "CONTEXT_ADJUSTMENT"};
    const auto parameters = QJsonDocument::fromJson(parametersJson.trimmed().isEmpty() ? QByteArray("{}") : parametersJson.toUtf8());
    const bool actionable = act == "EXECUTE" || act == "QUERY";
    if (!canCorrect() || !acts.contains(act) || !intents.contains(intent) || actionable == (intent == "NONE") ||
        parametersJson.size() > 2000 || !parameters.isObject()) {
        m_diagnostic = QStringLiteral("请检查语义类别、意图和参数 JSON");
        emit changed();
        return false;
    }
    auto record = m_last;
    record.insert("expected", QJsonObject{{"speech_act", act}, {"intent", intent}, {"slots", parameters.object()}});
    if (!original.trimmed().isEmpty())
        record.insert("original", original.trimmed().left(256));
    if (!trainingText.trimmed().isEmpty())
        record.insert("trainingText", trainingText.trimmed().left(500));
    const bool saved = append(record);
    m_diagnostic = saved ? QStringLiteral("已保存人工纠正") : QStringLiteral("人工纠正保存失败");
    emit changed();
    return saved;
}

bool AssistantNlu::append(const QJsonObject &record) {
    const auto path = qEnvironmentVariable("CAR_TERMINAL_NLU_LOG");
    if (path.isEmpty())
        return false;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append))
        return false;
    auto stamped = record;
    stamped.insert("at", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    const auto line = QJsonDocument(stamped).toJson(QJsonDocument::Compact) + '\n';
    return file.write(line) == line.size() && file.flush();
}

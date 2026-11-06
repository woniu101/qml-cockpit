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
#include <utility>

AssistantNlu::AssistantNlu(VehicleState &state, AssistantVehicle &vehicle, AssistantMedia &media, int followUpTimeoutMs)
    : m_state(state), m_vehicle(vehicle), m_media(media), m_batch(state, vehicle),
      m_worker(cockpit::InferenceKind::Nlu, cockpit::modelDirectory(cockpit::InferenceKind::Nlu)),
      m_candidate(cockpit::InferenceKind::Nlu, cockpit::modelDirectory(cockpit::InferenceKind::Nlu, true)) {
    m_diagnostic = m_worker.configured() ? QStringLiteral("rbtl3 · 只读查询与车控对照") : QStringLiteral("规则模式");
    connect(&m_worker, &AssistantInference::completed, this, &AssistantNlu::complete);
    connect(&m_batch, &AssistantBatch::changed, this, &AssistantNlu::changed);
    connect(&m_candidate, &AssistantInference::completed, this, [this](const QJsonObject &result) {
        if (m_candidateRevision != m_reviewRevision) {
            submitCandidate();
            return;
        }
        m_candidatePending = false;
        m_last.insert("candidateResult", result);
        if (m_windowPilot)
            finishWindowPilot(result);
        recordReview();
        emit changed();
    });
    m_followUpTimeout.setSingleShot(true);
    m_followUpTimeout.setInterval(followUpTimeoutMs);
    m_contextTimeout.setSingleShot(true);
    m_contextTimeout.setInterval(followUpTimeoutMs);
    connect(&m_contextTimeout, &QTimer::timeout, this, &AssistantNlu::clearContext);
    connect(&m_followUpTimeout, &QTimer::timeout, this, [this] {
        const bool modelWindow = m_followUp && m_followUp->modelWindow;
        if (modelWindow)
            markWindowPilot(QStringLiteral("expired"));
        clearFollowUp();
        m_media.answer(QStringLiteral("等待超时，请重新说出完整指令"), false);
        if (modelWindow)
            finishReview(QStringLiteral("model_window_expired"));
        emit changed();
    });
    connect(&state, &VehicleState::sourceChanged, this, [this] {
        if (m_windowPilot && !sourceMatches(m_windowPilotSource, m_windowPilotSession)) {
            m_windowPilot.reset();
            const auto message = QStringLiteral("车辆数据来源已变化，请重新发出指令");
            markWindowPilot(QStringLiteral("blocked"), message);
            m_media.answer(message, false);
            finishReview(QStringLiteral("model_window_blocked"));
            emit changed();
        }
        if (m_lastSetting && !sourceMatches(m_contextSource, m_contextSession))
            clearContext();
        if (m_followUp && !sourceMatches(m_followUpSource, m_followUpSession)) {
            const bool modelWindow = m_followUp->modelWindow;
            if (modelWindow)
                markWindowPilot(QStringLiteral("blocked"), QStringLiteral("车辆数据来源已变化"));
            clearFollowUp();
            m_media.answer(QStringLiteral("车辆数据来源已变化，请重新发出指令"), false);
            if (modelWindow)
                finishReview(QStringLiteral("model_window_blocked"));
            emit changed();
        }
    });
}

void AssistantNlu::submit(const QString &text, bool spoken) {
    const auto request = text.trimmed();
    if (request.isEmpty() || request.size() > 256)
        return;
    if (batchRunning()) {
        if (assistantCancelsRequest(request))
            cancelRemaining();
        return;
    }
    m_batch.clear();
    if (assistantCancelsRequest(request)) {
        if (m_windowPilot || (m_followUp && m_followUp->modelWindow)) {
            endConversation();
            m_media.answer(QStringLiteral("已取消本次请求"), spoken);
            finishReview(QStringLiteral("model_window_cancelled"));
            emit changed();
            return;
        }
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
    if (m_windowPilot)
        return;
    if (m_followUp) {
        if (handleFollowUp(request, spoken))
            return;
        clearFollowUp();
    }
    if (handleDialogue(request, spoken))
        return;
    if (beginWindowPilot(request, spoken))
        return;
    if (AssistantMedia::isTransportRequest(request)) {
        clearContext();
        m_superseded = m_active;
        beginReview(request, spoken);
        m_media.submit(request, spoken);
        m_diagnostic = QStringLiteral("媒体控制 · 直接解析");
        finishReview(QStringLiteral("transport_rules"));
        emit changed();
        return;
    }
    // Explicit temperature settings have a complete, validated command grammar.
    // Do not wait for a model before applying these deterministic commands.
    const auto direct = assistantVehicleSetting(request, spoken);
    if (direct && direct->kind == AssistantVehicleSetting::Kind::Temperature) {
        clearContext();
        m_superseded = m_active;
        beginReview(request, spoken);
        m_media.beginRequest(spoken);
        const bool clarifying = dispatchSetting(*direct, spoken);
        m_diagnostic = QStringLiteral("温度设定 · 直接解析");
        finishReview(clarifying ? QStringLiteral("clarify") : QStringLiteral("temperature_rules"));
        emit changed();
        return;
    }
    if (auto relative = assistantRelativeVehicleSetting(request)) {
        if (m_vehicle.awaitingWindow() || (busy() && assistantVehicleSetting(m_text, m_spoken))) {
            m_media.vehicleReply()(m_vehicle.awaitingWindow()
                ? QStringLiteral("上一条车窗指令仍在执行，请等待结果")
                : QStringLiteral("正在处理上一条车控指令，请等待结果"), false);
            return;
        }
        QJsonObject context;
        if (relative->target < 0 && m_lastSetting && m_lastSetting->kind == relative->kind) {
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
    if (m_active || !m_worker.submit({{"text", request}, {"input", spoken ? "asr" : "text"}})) {
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
    if (!m_followUp)
        return {};
    switch (m_followUp->kind) {
    case FollowUpKind::Target:
    case FollowUpKind::WindowQuery: return m_followUp->setting.targets();
    case FollowUpKind::ConfirmTarget: return {QStringLiteral("确认调整")};
    case FollowUpKind::BatteryQuery: return {QStringLiteral("查询车辆电量")};
    case FollowUpKind::Batch: return {QStringLiteral("确认执行全部")};
    case FollowUpKind::Value: return {};
    }
    return {};
}

QString AssistantNlu::followUpHint() const {
    if (!m_followUp)
        return {};
    switch (m_followUp->kind) {
    case FollowUpKind::Target:
    case FollowUpKind::WindowQuery: return QStringLiteral("输入位置，或说「取消」");
    case FollowUpKind::Value: return m_followUp->setting.kind == AssistantVehicleSetting::Kind::Window
        ? QStringLiteral("输入开度，如 30%") : QStringLiteral("输入温度，如 24 度");
    case FollowUpKind::ConfirmTarget:
    case FollowUpKind::BatteryQuery: return QStringLiteral("输入「确认」或「取消」");
    case FollowUpKind::Batch: return QStringLiteral("输入「确认执行」或「取消」");
    }
    return {};
}

bool AssistantNlu::handleFollowUp(const QString &request, bool spoken) {
    static const QRegularExpression confirmation(QStringLiteral(
        "^(?:确认|确定|是(?:的)?|对)[。！？!?]?$"));
    static const QRegularExpression adjust(QStringLiteral("^确认调整[。！？!?]?$"));
    static const QRegularExpression battery(QStringLiteral("^(?:是电量|车辆电量|查询车辆电量)[。！？!?]?$"));
    const auto pending = *m_followUp;
    if (pending.kind == FollowUpKind::Batch) {
        static const QRegularExpression approve(QStringLiteral("^确认(?:执行(?:全部)?)?[。！？!?]?$"));
        if (!approve.match(request).hasMatch())
            return false;
        finishFollowUp(0, spoken);
        return true;
    }
    if (pending.modelWindow) {
        static const QRegularExpression approve(QStringLiteral("^确认(?:调整)?[。！？!?]?$"));
        if (!approve.match(request).hasMatch())
            return false;
        finishFollowUp(0, spoken);
        return true;
    }
    const auto previous = m_last.value("text").toString();
    const auto review = [&] {
        beginReview(request, spoken, {{"previousText", previous}, {"result", "awaiting_clarification"}});
    };
    if (pending.kind == FollowUpKind::Value) {
        const auto value = assistantSettingValue(pending.setting.kind, request, spoken);
        if (!value)
            return false;
        auto setting = pending.setting;
        setting.value = *value;
        review();
        clearFollowUp();
        m_media.beginRequest(spoken);
        const bool clarifying = dispatchSetting(setting, spoken);
        finishReview(clarifying ? QStringLiteral("clarify") : QStringLiteral("value_reply"));
        emit changed();
        return true;
    }
    int selected = -1;
    if (pending.kind == FollowUpKind::ConfirmTarget || pending.kind == FollowUpKind::BatteryQuery) {
        if (confirmation.match(request).hasMatch() ||
            (pending.kind == FollowUpKind::ConfirmTarget ? adjust : battery).match(request).hasMatch())
            selected = 0;
    } else {
        selected = pending.setting.targetFromReply(request);
        if (const auto change = assistantRetarget(request); selected < 0 && change && change->kind == pending.setting.kind)
            selected = change->target;
    }
    if (selected < 0)
        return false;
    review();
    finishFollowUp(selected, spoken);
    finishReview(QStringLiteral("clarification_reply"));
    emit changed();
    return true;
}

bool AssistantNlu::handleDialogue(const QString &request, bool spoken) {
    static const QRegularExpression battery(QStringLiteral(
        "^(?:现在|当前)?(?:还剩|剩余|还剩余|剩)多少点[。！？!?]?$"));
    static const QRegularExpression windowQuery(QStringLiteral(
        "^(?:不对[，,]?)?(?:我问的是|我想问|我是问)(?:刚才那扇(?:车窗|窗)?)?(?:开到多少|开了多少|开度是多少)[。！？!?]?$"));
    static const QRegularExpression reviseValue(QStringLiteral(
        "^(?:不对[，,]?)?(温度|开度)(?:不是这个|不对)[，,]?(?:改一下|改一改)[。！？!?]?$"));
    const auto replacement = assistantRetarget(request);
    const auto batch = AssistantBatch::parse(request, spoken);
    const bool multi = batch.has_value() || assistantHasMultipleVehicleActions(request);
    const bool batteryQuestion = battery.match(request).hasMatch();
    const bool windowQuestion = windowQuery.match(request).hasMatch();
    const auto revision = reviseValue.match(request);
    if (!replacement && !multi && !batteryQuestion && !windowQuestion && !revision.hasMatch())
        return false;
    if (m_vehicle.awaitingWindow()) {
        m_media.vehicleReply()(QStringLiteral("上一条车窗指令仍在执行，请等待结果"), false);
        return true;
    }
    QJsonObject context;
    const auto last = m_lastSetting;
    const double lastValue = m_contextValue;
    if (last)
        context = {{"previousText", m_contextText}, {"result", "confirmed"},
                   {"target", last->targets().at(last->target)}, {"confirmedValue", lastValue}};
    clearContext();
    m_superseded = m_active;
    beginReview(request, spoken, context);
    if (batch && !batch->isEmpty()) {
        QStringList actions;
        for (const auto &setting : *batch)
            actions.append(AssistantBatch::describe(setting));
        FollowUp followUp{FollowUpKind::Batch};
        followUp.batch = *batch;
        startFollowUp(followUp, QStringLiteral("按顺序执行：%1。请确认。").arg(actions.join(QStringLiteral("，然后"))), spoken);
    } else if (multi) {
        m_media.answer(QStringLiteral("听到了多个调整，请先说要执行哪一项。"), spoken);
    } else if (batteryQuestion) {
        startFollowUp({FollowUpKind::BatteryQuery}, QStringLiteral("你是在问车辆还剩多少电吗？"), spoken);
    } else if (windowQuestion) {
        startFollowUp({FollowUpKind::WindowQuery, {AssistantVehicleSetting::Kind::Window, -1, 0}},
                      QStringLiteral("要查哪扇车窗的当前开度？"), spoken);
    } else if (revision.hasMatch()) {
        const bool window = revision.captured(1) == QStringLiteral("开度");
        const auto kind = window ? AssistantVehicleSetting::Kind::Window : AssistantVehicleSetting::Kind::Temperature;
        if (last && last->kind == kind) {
            const auto name = last->targets().at(last->target);
            startFollowUp({FollowUpKind::Value, {kind, last->target, lastValue}},
                          (window ? QStringLiteral("要把%1改到多少开度？") : QStringLiteral("要把%1改到多少度？")).arg(name), spoken);
        } else {
            m_media.answer(window ? QStringLiteral("请说出要调整的车窗和开度。")
                                  : QStringLiteral("请说出要调整的温区和温度。"), spoken);
        }
    } else if (!last || last->kind != replacement->kind) {
        m_media.answer(replacement->kind == AssistantVehicleSetting::Kind::Window
            ? QStringLiteral("请说出要调整的车窗和开度。") : QStringLiteral("请说出要调整的温区和温度。"), spoken);
    } else {
        const auto names = last->targets();
        const bool window = last->kind == AssistantVehicleSetting::Kind::Window;
        const auto original = names.at(last->target);
        const auto target = names.at(replacement->target);
        const auto actual = window ? m_state.windowActual().at(last->target).toDouble()
            : last->target == 1 ? m_state.passengerTemperature() : m_state.driverTemperature();
        if (last->target == replacement->target) {
            m_media.answer(window ? QStringLiteral("%1就是上次调整的车窗，当前开度 %2%。").arg(original).arg(actual)
                                  : QStringLiteral("%1就是上次调整的温区，当前设定 %2 度。").arg(original).arg(actual), spoken);
        } else {
            const bool sync = !window && m_state.climateSync();
            const auto prompt = window
                ? QStringLiteral("%1当前开度 %2%。要把%3也设为 %4% 吗？").arg(original).arg(actual).arg(target).arg(lastValue)
                : sync ? QStringLiteral("双区同步已开启。要把主副驾设为 %1 度吗？").arg(lastValue)
                       : QStringLiteral("%1当前设定 %2 度。要把%3设为 %4 度吗？").arg(original).arg(actual).arg(target).arg(lastValue);
            startFollowUp({FollowUpKind::ConfirmTarget, {last->kind, replacement->target, lastValue}, sync}, prompt, spoken);
        }
    }
    m_diagnostic = QStringLiteral("对话规则 · 处理改口或追问");
    finishReview(QStringLiteral("dialogue"));
    emit changed();
    return true;
}

bool AssistantNlu::dispatchRules(const QString &request, bool spoken) {
    if (const auto setting = assistantVehicleSetting(request, spoken))
        return dispatchSetting(*setting, spoken);
    m_media.submit(request, spoken);
    return false;
}

bool AssistantNlu::beginWindowPilot(const QString &request, bool spoken) {
    if (!m_windowPilotEnabled)
        return false;
    const auto setting = assistantWindowPilotEvidence(request, spoken);
    if (!setting)
        return false;
    if (m_vehicle.awaitingWindow()) {
        m_media.vehicleReply()(QStringLiteral("上一条车窗指令仍在执行，请等待结果"), false);
        return true;
    }
    clearContext();
    m_superseded = m_active;
    beginReview(request, spoken);
    m_media.beginRequest(spoken);
    if (setting->target < 0 || !setting->rangeError().isEmpty()) {
        dispatchSetting(*setting, spoken);
        m_diagnostic = QStringLiteral("单窗开度 · 参数待补全");
        finishReview(QStringLiteral("clarify"));
    } else if (!m_candidate.configured() || !m_state.externalReady()) {
        const auto message = m_state.externalReady() ? QStringLiteral("单窗模型暂不可用，请重新说「把车窗开到指定开度」")
            : QStringLiteral("模拟器未连接，无法调整车窗");
        m_media.answer(message, spoken);
        m_diagnostic = message;
        finishReview(QStringLiteral("model_window_blocked"));
    } else {
        m_windowPilot = setting;
        m_windowPilotSource = m_state.sourceMode();
        m_windowPilotSession = m_state.connection()->sessionId();
        m_windowPilotSpoken = spoken;
        m_last.insert("windowPilot", QJsonObject{{"target", setting->targets().at(setting->target)},
            {"opening", setting->value}, {"status", "checking"}});
        m_diagnostic = QStringLiteral("正在核对单窗模型提案…");
    }
    emit changed();
    return true;
}

void AssistantNlu::finishWindowPilot(const QJsonObject &result) {
    const auto setting = *m_windowPilot;
    m_windowPilot.reset();
    static const QStringList targets{"leftFront", "rightFront", "leftRear", "rightRear"};
    const auto proposal = result.value("proposal").toObject();
    const auto opening = proposal.value("value");
    QString reason;
    if (!sourceMatches(m_windowPilotSource, m_windowPilotSession))
        reason = QStringLiteral("车辆数据来源已变化");
    else if (result.value("model").toString() != m_candidateModelName || result.value("speech_act").toString() != "EXECUTE" ||
             result.value("intent").toString() != "WINDOW_SET")
        reason = QStringLiteral("模型未确认单窗开度指令");
    else if (result.value("confidence").toDouble() < 0.80)
        reason = QStringLiteral("模型置信度不足");
    else if (proposal.size() != 4 || proposal.value("action").toString() != "set" ||
             proposal.value("unit").toString() != "percent" || proposal.value("target").toString() != targets.at(setting.target) ||
             !opening.isDouble() || opening.toDouble() != setting.value)
        reason = QStringLiteral("模型的窗位或开度与原句不一致");
    if (!reason.isEmpty()) {
        markWindowPilot(QStringLiteral("blocked"), reason);
        m_diagnostic = reason;
        m_media.answer(reason + QStringLiteral("，请重新说出窗位和开度。"), m_windowPilotSpoken);
        finishReview(QStringLiteral("model_window_blocked"));
        return;
    }
    markWindowPilot(QStringLiteral("awaiting_confirmation"));
    startFollowUp({FollowUpKind::ConfirmTarget, setting, false, true},
        QStringLiteral("要把%1的开度设为 %2% 吗？").arg(setting.targets().at(setting.target)).arg(setting.value), m_windowPilotSpoken);
    m_diagnostic = QStringLiteral("单窗提案已核对 · 等待确认");
    finishReview(QStringLiteral("model_window_confirmation"));
}

void AssistantNlu::markWindowPilot(const QString &status, const QString &message) {
    auto audit = m_last.value("windowPilot").toObject();
    if (audit.isEmpty())
        return;
    audit.insert("status", status);
    if (!message.isEmpty())
        audit.insert("detail", message);
    m_last.insert("windowPilot", audit);
}

bool AssistantNlu::dispatchSetting(const AssistantVehicleSetting &setting, bool spoken) {
    if (const auto error = setting.rangeError(); !error.isEmpty()) {
        if (!setting.relative) {
            startFollowUp({FollowUpKind::Value, setting}, error + QStringLiteral("，请重新给出数值。"), spoken);
            return true;
        }
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
    startFollowUp({FollowUpKind::Target, setting}, setting.prompt(), spoken);
    return true;
}

void AssistantNlu::startFollowUp(const FollowUp &followUp, const QString &prompt, bool spoken) {
    m_followUp = followUp;
    m_followUpSource = m_state.sourceMode();
    m_followUpSession = m_state.connection()->sessionId();
    m_followUpSpoken = spoken;
    ++m_followUpRevision;
    if (!m_promptActive) m_followUpTimeout.start();
    m_media.answer(prompt, spoken);
}

void AssistantNlu::setPromptActive(bool active) {
    if (active == m_promptActive) return;
    m_promptActive = active;
    if (active) m_followUpTimeout.stop();
    else if (m_followUp) m_followUpTimeout.start();
}

void AssistantNlu::executeSetting(const AssistantVehicleSetting &setting, bool modelWindow) {
    const auto source = m_state.sourceMode();
    const auto session = m_state.connection()->sessionId();
    auto reply = m_media.vehicleReply();
    if (modelWindow) {
        markWindowPilot(QStringLiteral("executing"));
        reply = [self = QPointer<AssistantNlu>(this), revision = m_reviewRevision, respond = std::move(reply)]
                (const QString &message, bool final) {
            respond(message, final);
            if (!self || revision != self->m_reviewRevision)
                return;
            const bool confirmed = self->m_last.value("windowPilot").toObject().value("status").toString() == "confirmed";
            self->markWindowPilot(final ? (confirmed ? QStringLiteral("confirmed") : QStringLiteral("unconfirmed"))
                                        : QStringLiteral("executing"), message);
            if (final)
                self->finishReview(QStringLiteral("model_window_result"));
            emit self->changed();
        };
    }
    m_vehicle.execute(setting, reply, [self = QPointer<AssistantNlu>(this), modelWindow, review = m_reviewRevision,
        revision = m_contextRevision, source, session, setting, text = m_last.value("text").toString()] {
        if (!self || revision != self->m_contextRevision || !self->sourceMatches(source, session))
            return;
        if (modelWindow && review == self->m_reviewRevision)
            self->markWindowPilot(QStringLiteral("confirmed"));
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
    if (!m_followUp || target < 0 || target >= followUpOptions().size())
        return;
    const auto pending = *m_followUp;
    if (pending.modelWindow)
        markWindowPilot(QStringLiteral("executing"));
    clearFollowUp();
    if (pending.kind == FollowUpKind::Batch) {
        m_media.beginRequest(spoken);
        m_batch.start(pending.batch, m_media.vehicleReply());
        m_diagnostic = QStringLiteral("逐项执行车控");
        emit changed();
        return;
    } else if (pending.kind == FollowUpKind::BatteryQuery) {
        m_media.answer(assistantBatteryAnswer(m_state), spoken);
    } else if (pending.kind == FollowUpKind::WindowQuery) {
        m_media.answer(assistantWindowAnswer(m_state, target), spoken);
    } else if (pending.kind == FollowUpKind::ConfirmTarget &&
               pending.setting.kind == AssistantVehicleSetting::Kind::Temperature &&
               pending.climateSync != m_state.climateSync()) {
        m_media.answer(QStringLiteral("空调同步状态已变化，请重新说出温区和温度。"), spoken);
    } else {
        auto setting = pending.setting;
        if (pending.kind == FollowUpKind::Target)
            setting.target = target;
        m_media.beginRequest(spoken);
        executeSetting(setting, pending.modelWindow);
    }
    m_diagnostic = pending.modelWindow ? QStringLiteral("已确认 · 单窗模型提案")
                                      : QStringLiteral("已补全目标 · 规则执行");
    emit changed();
}

void AssistantNlu::clearFollowUp() {
    if (!m_followUp)
        return;
    if (m_followUp->modelWindow && m_last.value("windowPilot").toObject().value("status").toString() == "awaiting_confirmation")
        markWindowPilot(QStringLiteral("cancelled"));
    m_followUp.reset();
    ++m_followUpRevision;
    m_followUpTimeout.stop();
}

void AssistantNlu::cancelFollowUp() {
    if (!m_followUp)
        return;
    const bool modelWindow = m_followUp->modelWindow;
    clearFollowUp();
    m_media.answer(QStringLiteral("已取消本次请求"), false);
    if (modelWindow)
        finishReview(QStringLiteral("model_window_cancelled"));
    emit changed();
}

void AssistantNlu::endConversation() {
    m_batch.cancelRemaining();
    m_batch.clear();
    if (m_windowPilot) {
        m_windowPilot.reset();
        markWindowPilot(QStringLiteral("cancelled"));
    }
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
    const auto answer = assistantVehicleSetting(m_text, m_spoken) ? QString()
        : assistantModelVehicleAnswer(m_state, m_text, result);
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
        m_candidate.submit({{"text", m_last.value("text")}, {"input", m_last.value("input")}}))
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
    const auto status = m_last.value("windowPilot").toObject().value("status").toString();
    if (status == "awaiting_confirmation" || status == "executing")
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

#include "AssistantVehicle.h"
#include "AssistantNumbers.h"
#include "AssistantVehicleQuery.h"
#include "AssistantVehicleSetting.h"
#include "VehicleState.h"

#include <QStringList>
#include <algorithm>
#include <utility>

namespace {
const QStringList WindowPositions{QStringLiteral("左前"), QStringLiteral("右前"),
                                  QStringLiteral("左后"), QStringLiteral("右后")};
}

AssistantVehicle::AssistantVehicle(VehicleState &vehicle) : m_vehicle(vehicle) {
    m_deadline.setSingleShot(true);
    connect(&m_deadline, &QTimer::timeout, this, [this] { finish(windowUnconfirmed()); });
    connect(&vehicle, &VehicleState::windowProgressChanged, this, &AssistantVehicle::checkLocalWindow);
    connect(&vehicle, &VehicleState::sourceChanged, this, [this] {
        if (!m_reply)
            return;
        if ((!m_requestId.isEmpty() && (!m_vehicle.externalMode() || !m_vehicle.externalReady())) ||
            (!m_localSource.isEmpty() && m_localSource != m_vehicle.sourceMode()))
            finish(QStringLiteral("车辆数据源已变化，") + windowUnconfirmed());
    });
    connect(vehicle.connection(), &VehicleLink::snapshotReceived, this, [this] {
        if (m_requestId.isEmpty() || (!m_accepted && !m_completed))
            return;
        m_matchingSnapshot = windowsAtTarget();
        if (m_completed && m_matchingSnapshot)
            finish(windowSuccess(), true);
    });
    connect(vehicle.connection(), &VehicleLink::commandResult, this,
            [this](const QString &, const QJsonObject &, const QString &status, const QString &reason, const QString &id) {
        if (id.isEmpty() || id != m_requestId)
            return;
        if (status == "accepted") {
            m_accepted = true;
            m_reply(windowPending(), false);
        } else if (status == "completed") {
            m_completed = true;
            if (m_matchingSnapshot && windowsAtTarget())
                finish(windowSuccess(), true);
            else
                m_deadline.start(3000);
        } else {
            const QString operation = m_windowName + (m_windowTarget == 0 ? QStringLiteral("关闭") : QStringLiteral("调整"));
            const QString label = status == "rejected" ? operation + QStringLiteral("被拒绝")
                : status == "cancelled" ? operation + QStringLiteral("已取消")
                : status == "timeout" ? operation + QStringLiteral("超时") : windowUnconfirmed();
            finish(label + QStringLiteral("，") + reason);
        }
    });
}

bool AssistantVehicle::submit(const QString &request, Reply reply, bool spoken) {
    reset();
    static const QRegularExpression spacing(QStringLiteral("\\s+"));
    QString phrase = request;
    phrase.remove(spacing);
    const auto setting = assistantVehicleSetting(phrase, spoken);
    if (setting && setting->target >= 0) {
        execute(*setting, reply);
        return true;
    }
    if (handleWindows(phrase, reply) || handleClimate(phrase, reply))
        return true;

    const auto answer = assistantVehicleAnswer(m_vehicle, phrase);
    if (answer.isEmpty())
        return false;
    reply(answer, true);
    return true;
}

bool AssistantVehicle::handleWindows(const QString &phrase, const Reply &reply) {
    if (const auto closing = assistantWindowCloseTarget(phrase)) {
        requestWindow(*closing, 0, reply);
        return true;
    }

    return false;
}

void AssistantVehicle::execute(const AssistantVehicleSetting &setting, const Reply &reply, Confirmed confirmed) {
    reset();
    if (const auto error = setting.rangeError(); !error.isEmpty()) {
        reply(error, true);
        return;
    }
    if (setting.target < 0 || setting.target >= setting.targets().size())
        return;
    const bool window = setting.kind == AssistantVehicleSetting::Kind::Window;
    const bool passenger = setting.target == 1;
    double target = setting.value;
    if (setting.relative) {
        if (window && !m_vehicle.externalReady()) {
            reply(QStringLiteral("模拟器未连接，无法调整车窗"), true);
            return;
        }
        if (window && m_vehicle.windowBusy().at(setting.target).toBool()) {
            reply(QStringLiteral("车窗仍在移动，请等开度稳定后再调整"), true);
            return;
        }
        const double current = window ? m_vehicle.windowActual().at(setting.target).toDouble()
            : passenger ? m_vehicle.passengerTemperature() : m_vehicle.driverTemperature();
        target = window ? std::clamp(double(qRound(current)) + setting.value, 0.0, 100.0)
                        : std::clamp(current + setting.value, 16.0, 30.0);
        if (qFuzzyCompare(current, target)) {
            if (confirmed)
                confirmed();
            reply(window ? QStringLiteral("%1已达开度边界，当前为 %2%").arg(setting.targets().at(setting.target)).arg(current)
                         : QStringLiteral("%1空调已达温度边界，当前为 %2 度").arg(setting.targets().at(setting.target)).arg(current), true);
            return;
        }
    }
    if (window) {
        requestWindow(setting.target, int(target), reply, std::move(confirmed));
        return;
    }
    if (passenger)
        m_vehicle.setPassengerTemperature(target);
    else
        m_vehicle.setDriverTemperature(target);
    const auto actual = passenger ? m_vehicle.passengerTemperature() : m_vehicle.driverTemperature();
    const auto label = m_vehicle.climateSync() ? QStringLiteral("主副驾空调已同步设为 %1 度")
        : passenger ? QStringLiteral("副驾空调已设为 %1 度") : QStringLiteral("主驾空调已设为 %1 度");
    const bool success = qFuzzyCompare(actual, target);
    if (success && confirmed)
        confirmed();
    reply(success ? label.arg(actual) : QStringLiteral("空调温度设置未确认"), true);
}

bool AssistantVehicle::handleClimate(const QString &phrase, const Reply &reply) {
    static const QRegularExpression fan(QStringLiteral(
        "^(?:请帮我|请|帮我)?(?:把)?(?:空调)?风量(?:调到|调至|设为)([0-9零〇一二两三四五六七八九十]{1,2})[挡档]$"));
    const auto fanSetting = fan.match(phrase);
    if (fanSetting.hasMatch()) {
        const int target = assistantCommandInteger(fanSetting.captured(1));
        if (target < 0 || target > 7)
            reply(QStringLiteral("空调风量范围是 0 到 7 挡"), true);
        else {
            m_vehicle.setFan(target);
            reply(QStringLiteral("空调风量已设为 %1 挡").arg(m_vehicle.fan()), true);
        }
        return true;
    }

    static const QRegularExpression cooling(QStringLiteral(
        "^(?:请帮我|请|帮我)?(打开|开启|关闭|关掉)(?:空调制冷|制冷|A/C)$"), QRegularExpression::CaseInsensitiveOption);
    const auto ac = cooling.match(phrase);
    if (!ac.hasMatch())
        return false;
    const auto action = ac.captured(1);
    m_vehicle.setAcOn(action == QStringLiteral("打开") || action == QStringLiteral("开启"));
    reply(m_vehicle.acOn() ? QStringLiteral("空调制冷已开启") : QStringLiteral("空调制冷已关闭"), true);
    return true;
}

void AssistantVehicle::requestWindow(int index, int target, const Reply &reply, Confirmed confirmed) {
    m_windowIndex = index;
    m_windowTarget = target;
    m_windowName = index < 0 ? QStringLiteral("全部车窗") : WindowPositions.at(index) + QStringLiteral("车窗");
    if (!m_vehicle.externalReady()) {
        reply(QStringLiteral("模拟器未连接，无法操作%1").arg(m_windowName), true);
        return;
    }
    const bool external = m_vehicle.externalMode();
    if (!external && windowsAtTarget()) {
        if (confirmed)
            confirmed();
        reply(windowSuccess(), true);
        return;
    }
    m_requestId = index < 0 ? m_vehicle.closeWindows() : m_vehicle.setWindow(index, target);
    if (external && m_requestId.isEmpty()) {
        reply(QStringLiteral("%1指令未能发送，请检查模拟器连接或稍后重试").arg(m_windowName), true);
        return;
    }
    m_reply = reply;
    m_onConfirmed = std::move(confirmed);
    if (external) {
        m_reply(QStringLiteral("等待模拟器确认%1指令…").arg(m_windowName), false);
    } else {
        m_localSource = m_vehicle.sourceMode();
        m_deadline.start(6000);
        if (windowsAtTarget())
            finish(windowSuccess(), true);
        else
            m_reply(windowPending(), false);
    }
}

bool AssistantVehicle::windowsAtTarget() const {
    const auto actual = m_vehicle.windowActual();
    const auto busy = m_vehicle.windowBusy();
    for (int i = 0; i < WindowPositions.size(); ++i)
        if ((m_windowIndex < 0 || i == m_windowIndex) && (busy.at(i).toBool() || actual.at(i).toDouble() != m_windowTarget))
            return false;
    return true;
}

QString AssistantVehicle::windowSuccess() const {
    return m_windowTarget == 0 ? QStringLiteral("%1已关闭").arg(m_windowName)
                               : QStringLiteral("%1开度已调至 %2%").arg(m_windowName).arg(m_windowTarget);
}

QString AssistantVehicle::windowPending() const {
    return m_windowTarget == 0 ? QStringLiteral("正在关闭%1…").arg(m_windowName)
                               : QStringLiteral("正在将%1调至 %2%…").arg(m_windowName).arg(m_windowTarget);
}

QString AssistantVehicle::windowUnconfirmed() const {
    return QStringLiteral("%1%2未确认，请查看实际开度").arg(m_windowName,
        m_windowTarget == 0 ? QStringLiteral("关闭") : QStringLiteral("调整"));
}

void AssistantVehicle::checkLocalWindow() {
    if (!m_reply || m_localSource.isEmpty())
        return;
    const auto targets = m_vehicle.windowTargets();
    const auto busy = m_vehicle.windowBusy();
    bool anyBusy = false;
    for (int i = 0; i < WindowPositions.size(); ++i) {
        if (m_windowIndex >= 0 && i != m_windowIndex)
            continue;
        if (targets.at(i).toInt() != m_windowTarget) {
            finish(QStringLiteral("%1指令已被其他操作替代").arg(m_windowName));
            return;
        }
        anyBusy |= busy.at(i).toBool();
    }
    if (windowsAtTarget())
        finish(windowSuccess(), true);
    else if (!anyBusy)
        finish(windowUnconfirmed());
}

void AssistantVehicle::reset() {
    m_deadline.stop();
    m_reply = {};
    m_onConfirmed = {};
    m_requestId.clear();
    m_localSource.clear();
    m_accepted = m_completed = m_matchingSnapshot = false;
}

void AssistantVehicle::finish(const QString &message, bool confirmed) {
    auto reply = std::move(m_reply);
    auto onConfirmed = std::move(m_onConfirmed);
    reset();
    if (confirmed && onConfirmed)
        onConfirmed();
    if (reply)
        reply(message, true);
}

#pragma once

#include <QObject>
#include <QTimer>
#include <functional>

class VehicleState;
struct AssistantVehicleSetting;

class AssistantVehicle final : public QObject {
public:
    using Reply = std::function<void(const QString &, bool final)>;
    using Confirmed = std::function<void()>;

    explicit AssistantVehicle(VehicleState &vehicle);
    bool submit(const QString &request, Reply reply, bool spoken = false);
    void execute(const AssistantVehicleSetting &setting, const Reply &reply, Confirmed confirmed = {});
    bool awaitingWindow() const { return bool(m_reply); }
    void reset();

private:
    bool handleWindows(const QString &phrase, const Reply &reply);
    bool handleClimate(const QString &phrase, const Reply &reply);
    void requestWindow(int index, int target, const Reply &reply, Confirmed confirmed = {});
    bool windowsAtTarget() const;
    QString windowSuccess() const;
    QString windowPending() const;
    QString windowUnconfirmed() const;
    void checkLocalWindow();
    void finish(const QString &message, bool confirmed = false);

    VehicleState &m_vehicle;
    QTimer m_deadline;
    Reply m_reply;
    Confirmed m_onConfirmed;
    QString m_requestId, m_localSource;
    bool m_accepted = false;
    bool m_completed = false;
    bool m_matchingSnapshot = false;
    int m_windowIndex = -1;
    int m_windowTarget = 0;
    QString m_windowName;
};

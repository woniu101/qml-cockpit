#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <functional>

class MediaState;
class MediaSession;
class VideoLibrary;
class RadioService;

class AssistantMedia final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QVariantList choices READ choices NOTIFY changed)
    Q_PROPERTY(bool pending READ pending NOTIFY changed)
public:
    using Reply = std::function<void(const QString &, bool final)>;
    using VehicleHandler = std::function<bool(const QString &, bool spoken, Reply)>;
    AssistantMedia(MediaState &, MediaSession &, VideoLibrary &, VehicleHandler = {}, RadioService * = nullptr);
    QString message() const { return m_message; }
    QVariantList choices() const { return m_choices; }
    bool pending() const { return m_pending; }
    Q_INVOKABLE void submit(const QString &text, bool spoken = false);
    Q_INVOKABLE void choose(int index);
    Q_INVOKABLE void clearFeedback();
    bool chooseByRequest(const QString &request, bool spoken);
    void beginRequest(bool spoken);
    void answer(const QString &message, bool spoken);
    Reply vehicleReply();

signals:
    void changed();
    void viewRequested(const QString &source);
    void spokenReply(const QString &message);

private:
    QVariantList items() const;
    void play(const QVariantMap &item);
    bool handleMediaControl(const QString &request);
    bool handleRadio(const QString &request);
    void waitForPlayback(const QString &kind, const QString &url, bool openView = true);
    void confirmPlayback();
    void reply(const QString &message);
    MediaState &m_music;
    MediaSession &m_session;
    VideoLibrary &m_videos;
    VehicleHandler m_vehicleHandler;
    RadioService *m_radio;
    QTimer m_deadline;
    QVariantList m_choices;
    QString m_message, m_pendingUrl, m_pendingKind;
    quint64 m_pendingGeneration = 0;
    qint64 m_seekTarget = -1;
    bool m_openView = true;
    bool m_spoken = false;
    bool m_pending = false;
};

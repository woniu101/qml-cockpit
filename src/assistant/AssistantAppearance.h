#pragma once

#include <QImage>
#include <QObject>
#include <QTemporaryDir>
#include <QUrl>

class AssistantAppearance final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl avatar READ avatar NOTIFY changed)
    Q_PROPERTY(QUrl preview READ preview NOTIFY changed)
    Q_PROPERTY(bool custom READ custom NOTIFY changed)
    Q_PROPERTY(bool shown READ shown WRITE setShown NOTIFY changed)
    Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QUrl original READ original NOTIFY changed)
    Q_PROPERTY(QSize originalSize READ originalSize NOTIFY changed)
    Q_PROPERTY(double zoom READ zoom NOTIFY changed)
    Q_PROPERTY(double horizontal READ horizontal NOTIFY changed)
    Q_PROPERTY(double vertical READ vertical NOTIFY changed)

public:
    explicit AssistantAppearance(QString storage = {});
    QUrl avatar() const;
    QUrl preview() const { return m_preview; }
    bool custom() const { return m_custom; }
    bool shown() const { return m_shown; }
    bool reducedMotion() const { return m_reducedMotion; }
    QString message() const { return m_message; }
    QUrl original() const { return m_originalUrl; }
    QSize originalSize() const { return m_original.size(); }
    double zoom() const { return m_zoom; }
    double horizontal() const { return m_horizontal; }
    double vertical() const { return m_vertical; }
    void setShown(bool value);
    void setReducedMotion(bool value);
    Q_INVOKABLE bool load(const QUrl &url);
    Q_INVOKABLE void crop(double zoom, double horizontal, double vertical);
    Q_INVOKABLE bool save();
    Q_INVOKABLE void discard();
    Q_INVOKABLE void restoreDefault();

signals:
    void changed();

private:
    void status(const QString &message);
    QTemporaryDir m_temporary;
    QImage m_original, m_crop;
    QString m_storage, m_message;
    QUrl m_preview;
    QUrl m_originalUrl;
    double m_zoom = 1, m_horizontal = 50, m_vertical = 50;
    int m_revision = 0;
    bool m_custom = false, m_shown = true, m_reducedMotion = false;
    bool m_storePreferences = false;
};

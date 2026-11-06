#include "AssistantAppearance.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {
bool writePng(const QString &path, const QImage &image) {
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && image.save(&file, "PNG") && file.commit();
}

QUrl versioned(const QString &path, int revision) {
    QUrl url = QUrl::fromLocalFile(path);
    url.setQuery(QStringLiteral("revision=%1").arg(revision));
    return url;
}
}

AssistantAppearance::AssistantAppearance(QString storage) : m_storage(std::move(storage)) {
    m_storePreferences = !qEnvironmentVariableIsSet("CAR_TERMINAL_NO_SETTINGS");
    if (m_storage.isEmpty() && m_storePreferences)
        m_storage = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/assistant/avatar.png";
    m_custom = QFileInfo(m_storage).isFile();
    if (!m_storePreferences)
        return;
    QSettings settings;
    m_shown = settings.value("assistant/showAvatar", true).toBool();
    m_reducedMotion = settings.value("assistant/reducedMotion", false).toBool();
}

QUrl AssistantAppearance::avatar() const {
    return m_custom ? versioned(m_storage, m_revision) : QUrl("qrc:/assistant/character.png");
}

void AssistantAppearance::setShown(bool value) {
    if (m_shown == value)
        return;
    m_shown = value;
    if (m_storePreferences)
        QSettings().setValue("assistant/showAvatar", value);
    emit changed();
}

void AssistantAppearance::setReducedMotion(bool value) {
    if (m_reducedMotion == value)
        return;
    m_reducedMotion = value;
    if (m_storePreferences)
        QSettings().setValue("assistant/reducedMotion", value);
    emit changed();
}

bool AssistantAppearance::load(const QUrl &url) {
    const QFileInfo info(url.toLocalFile());
    if (!url.isLocalFile() || !info.isFile() || info.size() > 20 * 1024 * 1024 || !m_temporary.isValid()) {
        status(QStringLiteral("请选择不超过 20 MB 的本地图片"));
        return false;
    }
    QImageReader reader(info.absoluteFilePath());
    reader.setAutoTransform(true);
    const auto size = reader.size();
    const auto format = reader.format();
    if (!size.isValid() || qint64(size.width()) * size.height() > 48000000 ||
        (format != "jpeg" && format != "png")) {
        status(QStringLiteral("支持 PNG 和 JPG，请选择有效图片"));
        return false;
    }
    if (std::max(size.width(), size.height()) > 1600)
        reader.setScaledSize(size.scaled(1600, 1600, Qt::KeepAspectRatio));
    auto decoded = reader.read();
    if (decoded.isNull()) {
        status(QStringLiteral("无法读取该图片"));
        return false;
    }
    m_original = std::move(decoded);
    const auto originalPath = m_temporary.filePath("original.png");
    if (!writePng(originalPath, m_original)) {
        status(QStringLiteral("无法准备裁切图片"));
        return false;
    }
    m_originalUrl = versioned(originalPath, ++m_revision);
    crop(1, 50, 50);
    return !m_preview.isEmpty();
}

void AssistantAppearance::crop(double zoom, double horizontal, double vertical) {
    if (m_original.isNull() || !std::isfinite(zoom) || !std::isfinite(horizontal) || !std::isfinite(vertical))
        return;
    m_zoom = std::clamp(zoom, 1.0, 3.0);
    m_horizontal = std::clamp(horizontal, 0.0, 100.0);
    m_vertical = std::clamp(vertical, 0.0, 100.0);
    const int side = std::max(1, int(std::min(m_original.width(), m_original.height()) / std::clamp(zoom, 1.0, 3.0)));
    const int x = qRound((m_original.width() - side) * std::clamp(horizontal, 0.0, 100.0) / 100);
    const int y = qRound((m_original.height() - side) * std::clamp(vertical, 0.0, 100.0) / 100);
    const auto image = m_original.copy(x, y, side, side).scaled(384, 384, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    const auto path = m_temporary.filePath("preview.png");
    if (!writePng(path, image)) {
        status(QStringLiteral("无法生成裁切预览"));
        return;
    }
    m_crop = image;
    m_preview = versioned(path, ++m_revision);
    status(QStringLiteral("调整构图后，保存并使用"));
}

bool AssistantAppearance::save() {
    if (m_crop.isNull() || m_storage.isEmpty() ||
        !QDir().mkpath(QFileInfo(m_storage).absolutePath()) || !writePng(m_storage, m_crop)) {
        status(QStringLiteral("头像保存失败"));
        return false;
    }
    m_custom = true;
    ++m_revision;
    discard();
    status(QStringLiteral("自定义头像已启用"));
    return true;
}

void AssistantAppearance::discard() {
    m_original = {};
    m_crop = {};
    m_preview = {};
    m_originalUrl = {};
    m_zoom = 1;
    m_horizontal = m_vertical = 50;
    status({});
}

void AssistantAppearance::restoreDefault() {
    if (m_custom && QFileInfo::exists(m_storage) && !QFile::remove(m_storage)) {
        status(QStringLiteral("无法恢复默认头像"));
        return;
    }
    m_custom = false;
    discard();
    status(QStringLiteral("已恢复羊角帽形象"));
}

void AssistantAppearance::status(const QString &message) {
    m_message = message;
    emit changed();
}

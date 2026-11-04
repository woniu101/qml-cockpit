#include "AssistantMedia.h"
#include "AssistantNumbers.h"
#include "media/MediaSession.h"
#include "media/MediaState.h"
#include "media/VideoLibrary.h"
#include "media/RadioService.h"

#include <QFileInfo>
#include <QPointer>
#include <QRegularExpression>
#include <cmath>

namespace {
QString normalized(const QString &text) {
    static const QRegularExpression cjkSpacing(QStringLiteral("(?<=\\p{Han})\\s+|\\s+(?=\\p{Han})"));
    return text.normalized(QString::NormalizationForm_C).simplified().remove(cjkSpacing).toCaseFolded();
}

bool available(const QVariantMap &item) {
    const QUrl url(item.value("url").toString());
    return url.isLocalFile() && item.value("available", true).toBool() && QFileInfo(url.toLocalFile()).isFile();
}

}

AssistantMedia::AssistantMedia(MediaState &music, MediaSession &session, VideoLibrary &videos,
                               VehicleHandler vehicleHandler, RadioService *radio)
    : m_music(music), m_session(session), m_videos(videos), m_vehicleHandler(std::move(vehicleHandler)), m_radio(radio) {
    m_deadline.setSingleShot(true);
    m_deadline.setInterval(10000);
    connect(&m_deadline, &QTimer::timeout, this, [this] {
        reply(m_seekTarget >= 0 ? QStringLiteral("播放器暂未确认进度调整")
                                : QStringLiteral("播放器暂未确认开始播放，请查看媒体页"));
    });
    connect(&session, &MediaSession::changed, this, &AssistantMedia::confirmPlayback);
    connect(&session, &MediaSession::positionChanged, this, &AssistantMedia::confirmPlayback);
}

QVariantList AssistantMedia::items() const {
    QVariantList results;
    for (const auto &track : m_music.tracks()) {
        auto item = track.toMap();
        item.insert("kind", "music");
        results.append(item);
    }
    for (const auto &video : m_videos.playbackItems()) {
        auto item = video.toMap();
        item.insert("kind", "video");
        results.append(item);
    }
    return results;
}

void AssistantMedia::reply(const QString &message) {
    m_deadline.stop();
    m_pendingUrl.clear();
    m_seekTarget = -1;
    m_pending = false;
    m_message = message;
    emit changed();
    if (m_spoken)
        emit spokenReply(message);
}

void AssistantMedia::beginRequest(bool spoken) {
    m_deadline.stop();
    m_pendingUrl.clear();
    m_seekTarget = -1;
    m_choices.clear();
    m_spoken = spoken;
    m_pending = false;
    m_message.clear();
    emit changed();
}

void AssistantMedia::clearFeedback() {
    if (m_pending)
        return;
    m_message.clear();
    m_choices.clear();
    emit changed();
}

void AssistantMedia::answer(const QString &message, bool spoken) {
    beginRequest(spoken);
    reply(message);
}

AssistantMedia::Reply AssistantMedia::vehicleReply() {
    return [self = QPointer<AssistantMedia>(this)](const QString &message, bool final) {
        if (!self)
            return;
        if (final) {
            self->reply(message);
        } else {
            self->m_pending = true;
            self->m_message = message;
            emit self->changed();
        }
    };
}

void AssistantMedia::submit(const QString &text, bool spoken) {
    beginRequest(spoken);
    QString request = text.trimmed();
    while (!request.isEmpty() && QStringLiteral("。！？?!").contains(request.back()))
        request.chop(1);
    if (m_vehicleHandler && m_vehicleHandler(request, spoken, vehicleReply()))
        return;
    if (handleMediaControl(request) || handleRadio(request))
        return;
    static const QRegularExpression controls(QStringLiteral("^(?:请|帮我)?(暂停|继续)(?:播放)?(音乐|视频)?$"));
    const auto control = controls.match(request);
    if (control.hasMatch()) {
        const QString kind = m_session.kind();
        const QString source = control.captured(2);
        if ((kind != "music" && kind != "video") || (source == QStringLiteral("音乐") && kind != "music") ||
            (source == QStringLiteral("视频") && kind != "video")) {
            reply(QStringLiteral("当前没有对应的本地媒体会话"));
            return;
        }
        if (control.captured(1) == QStringLiteral("暂停")) {
            m_session.pause();
            reply(m_session.playing() ? QStringLiteral("播放器暂未确认暂停") : QStringLiteral("已暂停"));
            return;
        }
        const QString url = kind == "music" ? m_music.trackUrl() : m_session.itemId();
        if (url.isEmpty() || (kind == "video" && !m_session.videoAllowed())) {
            reply(QStringLiteral("当前媒体无法继续播放，请检查媒体与驻车状态"));
            return;
        }
        if (!m_session.playing()) {
            if (kind == "music")
                m_music.setPlaying(true);
            else
                m_session.toggle();
        }
        waitForPlayback(kind, url, false);
        return;
    }

    static const QRegularExpression playRequest(
        QStringLiteral("^(?:请帮我|请|帮我|给我)?(播放|放一首|我想听|我想看)\\s*(.+)$"));
    const auto match = playRequest.match(request);
    if (!match.hasMatch() || request.size() > 160) {
        reply(QStringLiteral("可以点播本地媒体、查询车辆状态、关闭左前车窗或设置主驾温度"));
        return;
    }
    const QString verb = match.captured(1);
    const QString name = normalized(match.captured(2));
    for (const auto &entry : items()) {
        const auto item = entry.toMap();
        if (!available(item))
            continue;
        const bool video = item.value("kind") == "video";
        if ((video && (verb == QStringLiteral("放一首") || verb == QStringLiteral("我想听"))) ||
            (!video && verb == QStringLiteral("我想看")))
            continue;
        const QString title = normalized(item.value("title").toString());
        const QString videoBase = video && title.endsWith("mv") && title.size() > 2 ? title.chopped(2).trimmed() : title;
        const QString artist = normalized(item.value("artist").toString());
        const QString baseName = QFileInfo(QUrl(item.value("url").toString()).toLocalFile()).completeBaseName();
        const QString fileTitle = normalized(baseName);
        const qsizetype separator = baseName.lastIndexOf(QStringLiteral(" - "));
        const bool fileNamePair = !video && artist.isEmpty() && separator > 0 &&
            (name == normalized(baseName.left(separator)) ||
             name == normalized(baseName.mid(separator + 3) + QStringLiteral("的") + baseName.left(separator)));
        const bool matches = (!title.isEmpty() && name == title) || name == fileTitle ||
            (video && !videoBase.isEmpty() && (name == videoBase || name == QStringLiteral("视频") + videoBase ||
                name == QStringLiteral("视频") + title || name == title + QStringLiteral("视频"))) ||
            (!video && !artist.isEmpty() && name == artist + QStringLiteral("的") + title) || fileNamePair;
        if (matches)
            m_choices.append(item);
    }
    if (m_choices.isEmpty()) {
        reply(QStringLiteral("本地媒体库没有找到匹配的文件，请先导入或检查名称"));
    } else if (m_choices.size() == 1) {
        choose(0);
    } else {
        reply(QStringLiteral("找到多个匹配，请选择要播放的一项"));
    }
}

bool AssistantMedia::handleRadio(const QString &request) {
    static const QRegularExpression radio(QStringLiteral(
        "^(?:请帮我|帮我|请)?(?:收听|切到|切换到)(?:(收藏)?电台)?(.{1,40})$"));
    const auto match = radio.match(normalized(request));
    if (!m_radio || !match.hasMatch())
        return false;
    const auto id = m_radio->stationIdForName(match.captured(2), !match.captured(1).isEmpty());
    if (id.isEmpty())
        reply(QStringLiteral("没有找到这个电台，请检查台名或收藏列表"));
    else if (m_session.kind() == "phone" || !m_session.outputAvailable())
        reply(QStringLiteral("请先断开手机音乐并连接音频输出"));
    else {
        if (m_session.kind() != "radio" || m_session.itemId() != id || !m_radio->active())
            m_radio->tune(id);
        waitForPlayback("radio", id);
    }
    return true;
}

bool AssistantMedia::handleMediaControl(const QString &request) {
    static const QRegularExpression skip(QStringLiteral("^(?:请|帮我)?(?:切到|播放|切换到)?(上一首|下一首)(?:歌)?$"));
    const auto track = skip.match(request);
    if (track.hasMatch()) {
        if (m_session.kind() != "music" || !m_session.outputAvailable()) {
            reply(QStringLiteral("请先打开本地音乐并连接音频输出"));
        } else {
            const auto before = m_music.trackUrl();
            m_music.nextTrack(track.captured(1) == QStringLiteral("上一首") ? -1 : 1);
            const auto after = m_music.trackUrl();
            if (after.isEmpty() || before == after)
                reply(QStringLiteral("当前队列没有切换到其他曲目"));
            else
                waitForPlayback("music", after, false);
        }
        return true;
    }

    static const QRegularExpression volume(QStringLiteral(
        "^(?:请|帮我)?(?:把)?(?:媒体)?音量(调到|调至|设为|提高|降低|调高|调低)(?:百分之)?([0-9零〇一二两三四五六七八九十百]{1,5})%?$"));
    const auto level = volume.match(request);
    if (level.hasMatch()) {
        const int value = assistantCommandInteger(level.captured(2));
        const QString mode = level.captured(1);
        const bool up = mode == QStringLiteral("提高") || mode == QStringLiteral("调高");
        const bool down = mode == QStringLiteral("降低") || mode == QStringLiteral("调低");
        const int target = up ? m_music.volume() + value : down ? m_music.volume() - value : value;
        if (m_session.kind() == "phone")
            reply(QStringLiteral("手机蓝牙音量请在手机上调节"));
        else if (value < 0 || target < 0 || target > 100)
            reply(QStringLiteral("媒体音量范围是 0 到 100%"));
        else {
            m_music.setVolume(target);
            reply(QStringLiteral("媒体音量 %1%").arg(m_music.volume()));
        }
        return true;
    }

    static const QRegularExpression seek(QStringLiteral(
        "^(?:请|帮我)?(音乐|视频)?(快进|快退)([0-9零〇一二两三四五六七八九十百]{1,5})(秒|分钟)$"));
    const auto offset = seek.match(request);
    if (!offset.hasMatch())
        return false;
    const QString kind = m_session.kind();
    const QString source = offset.captured(1);
    if ((kind != "music" && kind != "video") || !m_session.canSeek() || m_session.busy() ||
        (kind == "video" && !m_session.videoAllowed()) ||
        (source == QStringLiteral("音乐") && kind != "music") || (source == QStringLiteral("视频") && kind != "video")) {
        reply(QStringLiteral("当前媒体进度不可调整"));
        return true;
    }
    const int amount = assistantCommandInteger(offset.captured(3));
    const qint64 scale = offset.captured(4) == QStringLiteral("分钟") ? 60000 : 1000;
    const qint64 delta = amount * scale * (offset.captured(2) == QStringLiteral("快退") ? -1 : 1);
    const qint64 target = qRound64(m_session.position()) + delta;
    if (amount <= 0 || target < 0 || target >= m_session.duration()) {
        reply(QStringLiteral("调整后的进度超出当前媒体范围"));
        return true;
    }
    m_seekTarget = target;
    waitForPlayback(kind, kind == "music" ? m_music.trackUrl() : m_session.itemId(), false);
    m_session.seek(target);
    confirmPlayback();
    return true;
}

bool AssistantMedia::chooseByRequest(const QString &request, bool spoken) {
    static const QRegularExpression ordinal(QStringLiteral(
        "^(?:请|帮我)?(?:选|选择|播放)?第([0-9一二两三四五六七八九十]{1,2})(?:个|项|首)?[。！!]?$"));
    if (m_choices.isEmpty())
        return false;
    const auto match = ordinal.match(normalized(request));
    if (!match.hasMatch())
        return false;
    const int index = assistantCommandInteger(match.captured(1)) - 1;
    if (index < 0 || index >= m_choices.size()) {
        reply(QStringLiteral("没有这一项，请按列表中的编号选择"));
        return true;
    }
    m_spoken = spoken;
    choose(index);
    return true;
}

void AssistantMedia::choose(int index) {
    if (index < 0 || index >= m_choices.size())
        return;
    const auto selected = m_choices.at(index).toMap();
    m_choices.clear();
    for (const auto &entry : items()) {
        const auto item = entry.toMap();
        if (item.value("url") == selected.value("url") && item.value("kind") == selected.value("kind") && available(item)) {
            play(item);
            return;
        }
    }
    reply(QStringLiteral("此媒体已不可用，请重新选择"));
}

void AssistantMedia::play(const QVariantMap &item) {
    const QString kind = item.value("kind").toString();
    const QString url = item.value("url").toString();
    if (m_session.kind() == "phone") {
        reply(QStringLiteral("请先断开手机音乐，再播放本地媒体"));
        return;
    }
    if (kind == "video" && !m_session.videoAllowed()) {
        reply(QStringLiteral("请驻车后播放视频"));
        return;
    }
    if (kind == "music" && !m_session.outputAvailable()) {
        reply(QStringLiteral("没有可用音频输出，请连接扬声器或耳机"));
        return;
    }
    if (kind == "music") {
        const auto tracks = m_music.tracks();
        for (int index = 0; index < tracks.size(); ++index) {
            if (tracks.at(index).toMap().value("url").toString() == url) {
                m_session.useMusic();
                if (m_music.track() == index)
                    m_music.setPlaying(true);
                else
                    m_music.selectTrack(index);
                break;
            }
        }
    } else {
        m_videos.play(url);
    }
    waitForPlayback(kind, url);
}

void AssistantMedia::waitForPlayback(const QString &kind, const QString &url, bool openView) {
    m_pending = true;
    m_openView = openView;
    m_pendingKind = kind;
    m_pendingUrl = url;
    m_pendingGeneration = m_session.generation();
    m_message = kind == "radio" ? QStringLiteral("正在连接电台…")
        : m_seekTarget >= 0 ? QStringLiteral("正在调整媒体进度…") : QStringLiteral("等待播放器确认…");
    m_deadline.start();
    emit changed();
    confirmPlayback();
}

void AssistantMedia::confirmPlayback() {
    if (m_pendingUrl.isEmpty())
        return;
    const QString active = m_pendingKind == "music" ? m_music.trackUrl() : m_session.itemId();
    if (m_session.generation() != m_pendingGeneration || m_session.kind() != m_pendingKind || active != m_pendingUrl) {
        reply(QStringLiteral("播放请求已被其他媒体操作替代"));
    } else if (m_session.phase() == "blocked") {
        reply(QStringLiteral("请驻车后播放视频"));
    } else if (m_session.phase() == "error") {
        reply(QStringLiteral("播放失败：%1").arg(m_session.error()));
    } else if (m_seekTarget >= 0) {
        if (std::abs(m_session.position() - m_seekTarget) < 450)
            reply(QStringLiteral("已跳到 %1 秒").arg(qRound(m_session.position() / 1000.0)));
    } else if (!m_session.outputAvailable() && (m_pendingKind == "music" || m_session.hasAudio())) {
        reply(QStringLiteral("没有可用音频输出，请连接扬声器或耳机"));
    } else if ((m_pendingKind == "radio" ? m_session.player()->hasAudio() : m_session.player()->source() == QUrl(m_pendingUrl)) &&
               m_session.playing() && m_session.player()->position() > 0) {
        const QString kind = m_pendingKind;
        reply((kind == "radio" ? QStringLiteral("正在收听 %1") : QStringLiteral("正在播放 %1")).arg(m_session.title()));
        if (m_openView)
            emit viewRequested(kind);
    }
}

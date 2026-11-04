#include "AssistantWorker.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <utility>

AssistantWorker::AssistantWorker(QString python, QString script, QString model, int timeoutMs)
    : m_python(std::move(python)), m_script(std::move(script)), m_model(std::move(model)) {
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(timeoutMs);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        m_process.kill();
        finish({{"error", "worker timeout"}});
    });
    connect(&m_process, &QProcess::started, this, &AssistantWorker::writeRequest);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this] {
        while (m_process.canReadLine()) {
            const auto line = m_process.readLine(65537);
            const auto result = QJsonDocument::fromJson(line).object();
            finish(result.isEmpty() ? QJsonObject{{"error", "invalid worker response"}} : result);
        }
        if (m_process.bytesAvailable() > 65536) {
            m_process.kill();
            finish({{"error", "worker response too large"}});
        }
    });
    connect(&m_process, &QProcess::readyReadStandardError, this, [this] {
        const auto line = m_process.readAllStandardError().trimmed();
        if (!line.isEmpty())
            qWarning("Assistant worker: %s", line.left(700).constData());
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            finish({{"error", m_process.errorString()}});
    });
    connect(&m_process, &QProcess::finished, this, [this] {
        finish({{"error", "worker exited"}});
    });
}

AssistantWorker::~AssistantWorker() {
    m_process.disconnect(this);
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

bool AssistantWorker::configured() const {
    return QFileInfo(m_python).isFile() && QFileInfo(m_script).isFile() && QFileInfo(m_model).isDir();
}

bool AssistantWorker::submit(const QJsonObject &request) {
    if (m_busy || !configured())
        return false;
    m_busy = true;
    m_pending = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    m_timeout.start();
    if (m_process.state() == QProcess::NotRunning)
        m_process.start(m_python, {"-I", "-B", "-u", m_script, m_model});
    else
        writeRequest();
    return true;
}

void AssistantWorker::writeRequest() {
    if (!m_pending.isEmpty()) {
        m_process.write(m_pending);
        m_pending.clear();
    }
}

void AssistantWorker::finish(const QJsonObject &result) {
    if (!m_busy)
        return;
    m_busy = false;
    m_pending.clear();
    m_timeout.stop();
    emit completed(result);
}

#include "WatchdogProcess.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>

WatchdogProcess::WatchdogProcess(QObject* parent)
    : QObject(parent)
{
    m_heartbeatTimer.setInterval(100);  // 每 100ms 检查心跳
    connect(&m_heartbeatTimer, &QTimer::timeout,
            this, &WatchdogProcess::onHeartbeatTimeout);
}

WatchdogProcess::~WatchdogProcess()
{
    if (m_client) {
        m_client->disconnect();
        delete m_client;
        m_client = nullptr;
    }
    m_server.close();
}

bool WatchdogProcess::start(const QString& socketName)
{
    // 清理旧连接
    QLocalServer::removeServer(socketName);

    if (!m_server.listen(socketName)) {
        std::cerr << "[Watchdog] Failed to listen on " << socketName.toStdString() << "\n";
        return false;
    }

    connect(&m_server, &QLocalServer::newConnection,
            this, &WatchdogProcess::onNewConnection);

    m_heartbeatTimer.start();
    m_lastHeartbeat = QDateTime::currentDateTime();

    std::cout << "[Watchdog] Listening on '" << socketName.toStdString() << "'\n";
    return true;
}

void WatchdogProcess::onNewConnection()
{
    if (m_client) {
        m_client->disconnect();
        m_client->deleteLater();
    }

    m_client = m_server.nextPendingConnection();
    m_clientConnected = true;
    m_lastHeartbeat = QDateTime::currentDateTime();

    connect(m_client, &QLocalSocket::readyRead, this, &WatchdogProcess::onReadyRead);
    connect(m_client, &QLocalSocket::disconnected,
            this, &WatchdogProcess::onClientDisconnected);

    std::cout << "[Watchdog] Main process connected\n";
}

void WatchdogProcess::onReadyRead()
{
    while (m_client && m_client->canReadLine()) {
        QByteArray line = m_client->readLine();
        parseHeartbeat(line);
    }
}

void WatchdogProcess::onClientDisconnected()
{
    std::cerr << "[Watchdog] Main process disconnected!\n";
    m_clientConnected = false;
    m_client = nullptr;
    m_safetyMode = true;
    std::cout << "[Watchdog] SAFETY MODE activated (connection lost)\n";
}

void WatchdogProcess::onHeartbeatTimeout()
{
    if (!m_clientConnected) return;

    auto elapsed = m_lastHeartbeat.msecsTo(QDateTime::currentDateTime());
    if (elapsed > 500 && !m_safetyMode) {
        m_safetyMode = true;
        m_safetyPrinted = false;
        std::cout << "[Watchdog] SAFETY MODE activated (heartbeat timeout: "
                  << elapsed << "ms)\n";
    } else if (elapsed <= 500 && m_safetyMode) {
        m_safetyMode = false;
        m_safetyPrinted = false;
        std::cout << "[Watchdog] Safety mode cleared, main process recovered\n";
    }

    if (m_safetyMode && !m_safetyPrinted) {
        std::cout << "[Watchdog] SAFETY DISPLAY: Speed=" << m_safeSpeed
                  << " km/h, L=" << (m_safeLeftTurn ? "ON" : "OFF")
                  << ", R=" << (m_safeRightTurn ? "ON" : "OFF")
                  << " | SYSTEM ERROR - SAFETY MODE\n";
        m_safetyPrinted = true;
    }
}

void WatchdogProcess::parseHeartbeat(const QByteArray& data)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return;

    QJsonObject obj = doc.object();
    m_safeSpeed = obj["speed"].toDouble();
    m_safeLeftTurn = obj["leftTurn"].toBool();
    m_safeRightTurn = obj["rightTurn"].toBool();
    m_lastHeartbeat = QDateTime::currentDateTime();
}

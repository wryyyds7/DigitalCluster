#include "HeartbeatSender.h"
#include <QJsonDocument>
#include <QJsonObject>

HeartbeatSender::HeartbeatSender(QObject* parent)
    : QObject(parent)
{
    m_timer.setInterval(200);  // 200ms 心跳
    connect(&m_timer, &QTimer::timeout, this, &HeartbeatSender::onTimeout);
}

HeartbeatSender::~HeartbeatSender()
{
    stop();
    if (m_socket) {
        m_socket->disconnectFromServer();
        // 不需要 delete：m_socket 的 parent 是 this，
        // Qt 对象树会在 HeartbeatSender 析构时自动释放 m_socket
    }
}

void HeartbeatSender::start(const QString& socketName)
{
    m_socketName = socketName;
    m_timer.start();
}

void HeartbeatSender::stop()
{
    m_timer.stop();
}

void HeartbeatSender::onTimeout()
{
    if (!m_provider) return;

    // 延迟创建 socket
    if (!m_socket) {
        m_socket = new QLocalSocket(this);
    }

    // 异步连接：不阻塞 UI 线程
    // 连接中或未连接时发起 connectToServer，下一 tick 再发数据
    if (m_socket->state() != QLocalSocket::ConnectedState) {
        if (m_socket->state() == QLocalSocket::UnconnectedState) {
            m_socket->connectToServer(m_socketName);
        }
        return;  // 等下个 tick 确认连接后再发
    }

    auto [speed, leftTurn, rightTurn] = m_provider();

    QJsonObject obj;
    obj["speed"] = speed;
    obj["leftTurn"] = leftTurn;
    obj["rightTurn"] = rightTurn;

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    m_socket->write(data);
    m_socket->flush();
}

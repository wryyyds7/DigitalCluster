#pragma once

#include <QObject>
#include <QTimer>
#include <QLocalSocket>
#include <functional>

/**
 * @brief 主进程端的心跳发送器。
 *
 * 每 200ms 从 ViewModel 获取最新关键信号（车速、转向灯），
 * 通过 IPC 发送给 WatchdogProcess。
 *
 * QLocalSocket 作为成员变量管理，随 HeartbeatSender 生命周期销毁，
 * 避免 static 指针导致的内存泄漏和线程安全问题。
 */
class HeartbeatSender : public QObject
{
    Q_OBJECT

public:
    using DataProvider = std::function<std::tuple<double, bool, bool>()>;

    explicit HeartbeatSender(QObject* parent = nullptr);
    ~HeartbeatSender() override;

    /// 设置数据提供回调
    void setDataProvider(DataProvider provider) { m_provider = std::move(provider); }

    /// 启动心跳发送
    void start(const QString& socketName = "DigitalClusterWatchdog");

    /// 停止
    void stop();

private slots:
    void onTimeout();

private:
    QTimer        m_timer;
    QString       m_socketName;
    DataProvider  m_provider;
    QLocalSocket* m_socket = nullptr;  ///< IPC 连接，成员变量管理生命周期
};

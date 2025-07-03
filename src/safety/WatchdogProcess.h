#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QDateTime>

/**
 * @brief 看门狗进程（独立可执行文件 ClusterWatchdog）。
 *
 * 功能安全模拟方案：
 *   1. 主进程 (DigitalCluster) 启动后通过本地 socket 连接 Watchdog
 *   2. 主进程每 200ms 发送心跳（包含最新车速、转向灯等关键信号）
 *   3. Watchdog 如果 500ms 未收到心跳，判定主进程 Crash
 *   4. Watchdog 自动弹出独立的安全窗口，显示最低限度安全信息：
 *      - 车速
 *      - 转向灯状态
 *      - "SYSTEM ERROR - SAFETY MODE" 警告
 *
 * 这个设计模拟了车规级 ASIL-B/D 的功能安全降级要求。
 */
class WatchdogProcess : public QObject
{
    Q_OBJECT

public:
    explicit WatchdogProcess(QObject* parent = nullptr);
    ~WatchdogProcess() override;

    /// 启动看门狗服务
    bool start(const QString& socketName = "DigitalClusterWatchdog");

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();
    void onHeartbeatTimeout();

private:
    void parseHeartbeat(const QByteArray& data);

    QLocalServer  m_server;
    QLocalSocket* m_client = nullptr;
    QTimer        m_heartbeatTimer;

    QDateTime     m_lastHeartbeat;
    bool          m_clientConnected = false;

    // 安全模式数据
    double        m_safeSpeed = 0.0;
    bool          m_safeLeftTurn = false;
    bool          m_safeRightTurn = false;
    bool          m_safetyMode = false;
};

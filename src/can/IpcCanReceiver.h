#pragma once

#include "CanInterface.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <string>

/**
 * @brief IPC CAN 接收器
 *
 * 通过 Unix Domain Socket 连接 SignalGateway 中间件，
 * 接收 JSON 格式的信号数据，解析后通过 CanFrame 回调投递给上层。
 *
 * 协议：每行一个 JSON:
 *   {"name":"VehicleSpeed","value":120.0,"ts":1234567890}
 *
 * 收到的信号被包装为 CanFrame（id=0，data 存 JSON 原文），
 * 或直接通过扩展回调投递。这里复用 CanInterface 的 notifyFrame。
 */
class IpcCanReceiver : public CanInterface
{
public:
    IpcCanReceiver();
    ~IpcCanReceiver() override;

    bool open(const std::string& channel) override;  // channel = socket path
    void close() override;
    bool send(const CanFrame& frame) override;

private:
    void rxThreadFunc();
    void parseLine(const std::string& line);

    std::atomic<bool> m_running{false};
    std::thread       m_rxThread;
    int               m_sockFd = -1;
    std::string       m_socketPath;
};

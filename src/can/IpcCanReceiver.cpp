#include "IpcCanReceiver.h"
#include <iostream>
#include <cstring>
#include <sstream>

IpcCanReceiver::IpcCanReceiver() {}

IpcCanReceiver::~IpcCanReceiver()
{
    close();
}

bool IpcCanReceiver::open(const std::string& channel)
{
    m_socketPath = channel;

    m_sockFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_sockFd < 0) {
        std::cerr << "[IpcCanReceiver] socket() failed\n";
        return false;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, channel.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(m_sockFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[IpcCanReceiver] connect(" << channel << ") failed\n";
        ::close(m_sockFd);
        m_sockFd = -1;
        return false;
    }

    m_open.store(true);
    m_running.store(true);
    m_rxThread = std::thread(&IpcCanReceiver::rxThreadFunc, this);

    std::cout << "[IpcCanReceiver] Connected to " << channel << "\n";
    return true;
}

void IpcCanReceiver::close()
{
    m_running.store(false);
    m_open.store(false);

    if (m_sockFd >= 0) {
        ::close(m_sockFd);
        m_sockFd = -1;
    }

    if (m_rxThread.joinable()) m_rxThread.join();
}

bool IpcCanReceiver::send(const CanFrame& frame)
{
    // IPC 模式下不支持发送
    return false;
}

void IpcCanReceiver::rxThreadFunc()
{
    char buf[4096];
    std::string lineBuffer;

    while (m_running.load() && m_sockFd >= 0) {
        ssize_t n = read(m_sockFd, buf, sizeof(buf));
        if (n <= 0) {
            if (m_running.load()) {
                std::cerr << "[IpcCanReceiver] Connection lost\n";
            }
            break;
        }

        lineBuffer.append(buf, n);

        // 按行处理
        size_t pos;
        while ((pos = lineBuffer.find('\n')) != std::string::npos) {
            std::string line = lineBuffer.substr(0, pos);
            lineBuffer.erase(0, pos + 1);
            if (!line.empty()) {
                parseLine(line);
            }
        }
    }
}

void IpcCanReceiver::parseLine(const std::string& line)
{
    // 简化 JSON 解析：提取 name 和 value
    // 格式: {"name":"VehicleSpeed","value":120.0,"ts":1234567890}

    // 提取 name
    size_t nameStart = line.find("\"name\":\"");
    if (nameStart == std::string::npos) return;
    nameStart += 8;
    size_t nameEnd = line.find("\"", nameStart);
    if (nameEnd == std::string::npos) return;
    std::string name = line.substr(nameStart, nameEnd - nameStart);

    // 提取 value
    size_t valStart = line.find("\"value\":", nameEnd);
    if (valStart == std::string::npos) return;
    valStart += 8;
    size_t valEnd = line.find_first_of(",}", valStart);
    if (valEnd == std::string::npos) return;
    std::string valStr = line.substr(valStart, valEnd - valStart);

    // 构造 CanFrame：把信号名和值编码到 data 中
    // 用简单的格式：name=value\0
    CanFrame frame;
    frame.id = 0;
    std::string payload = name + "=" + valStr;
    frame.data.assign(payload.begin(), payload.end());
    frame.dlc = static_cast<uint8_t>(frame.data.size());
    frame.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch());

    notifyFrame(frame);
}

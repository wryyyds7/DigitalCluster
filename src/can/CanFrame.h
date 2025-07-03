#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <chrono>

/**
 * @brief CAN 帧结构体，表示一条原始 CAN 报文。
 *
 * 遵循标准 CAN 2.0A 格式：
 *   - id:      CAN 标识符（标准帧 11 bit / 扩展帧 29 bit）
 *   - dlc:     数据长度（0–8 字节，CAN-FD 最大 64）
 *   - data:    报文数据载荷
 *   - ext:     是否为扩展帧
 *   - rtr:     远程传输请求帧
 *   - timestamp: 接收时间戳（纳秒精度）
 */
struct CanFrame
{
    uint32_t           id        = 0;
    uint8_t            dlc       = 0;
    std::vector<uint8_t> data;
    bool               ext       = false;
    bool               rtr       = false;
    std::chrono::nanoseconds timestamp{0};

    CanFrame() = default;

    CanFrame(uint32_t id_, std::vector<uint8_t> data_)
        : id(id_), dlc(static_cast<uint8_t>(data_.size())),
          data(std::move(data_)) {}

    /// 将 data 以 Hex 字符串返回，便于调试
    std::string toHex() const
    {
        std::string s;
        s.reserve(data.size() * 3);
        for (auto b : data) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02X ", b);
            s += buf;
        }
        return s;
    }
};

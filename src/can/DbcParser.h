#pragma once

#include "CanFrame.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

/**
 * @brief DBC 信号定义
 *
 * 描述 CAN 报文中一个信号的位置、缩放、偏移等参数，
 * 与 Vector DBC 文件格式一一对应。
 */
struct DbcSignal
{
    std::string name;
    uint32_t    messageId = 0;
    uint8_t     startBit  = 0;    ///< 起始位（Intel: LSB，Motorola: MSB）
    uint8_t     bitLength = 0;
    bool        isLittleEndian = true;
    bool        isSigned = false;
    double      factor   = 1.0;  ///< 缩放因子
    double      offset   = 0.0;  ///< 偏移量
    double      minimum  = 0.0;
    double      maximum  = 0.0;
    std::string unit;
    uint8_t     receiver = 0;     ///< 接收节点

    /// 从原始 data 中提取信号值并转换为物理值
    double extract(const std::vector<uint8_t>& data) const;
};

/**
 * @brief DBC 报文定义
 */
struct DbcMessage
{
    uint32_t              id = 0;
    std::string           name;
    uint8_t               dlc = 0;
    std::string           sender;
    std::vector<DbcSignal> signals;
};

/**
 * @brief DBC 解析库
 *
 * 方案 A（推荐）: 解析标准 .dbc 文本文件。
 * 方案 B（内置）: 使用硬编码的信号定义，无需外部文件。
 *
 * 本实现提供方案 B 的内置 DBC 定义 + 可选的 .dbc 文件加载。
 */
class DbcParser
{
public:
    DbcParser();

    /// 加载 .dbc 文件
    bool loadFromFile(const std::string& path);

    /// 注册内置 DBC 定义
    void loadBuiltinDatabase();

    /// 解析一帧 CAN 报文，输出所有信号的物理值
    /// 返回 {signalName: physicalValue}
    std::unordered_map<std::string, double> parseFrame(const CanFrame& frame) const;

    /// 获取指定信号值（不存在返回 NaN）
    double getSignal(const CanFrame& frame, const std::string& signalName) const;

    /// 获取所有报文定义
    const std::unordered_map<uint32_t, DbcMessage>& messages() const { return m_messages; }

    /// 获取所有信号名 → 报文ID 映射（用于快速查找）
    const std::unordered_map<std::string, uint32_t>& signalMap() const { return m_signalToMsg; }

private:
    void registerSignal(const DbcSignal& sig);

    std::unordered_map<uint32_t, DbcMessage> m_messages;
    std::unordered_map<std::string, uint32_t> m_signalToMsg;
};

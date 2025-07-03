#include "DbcParser.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// ── DbcSignal: 从 raw data 中提取物理值 ─────────────────────

double DbcSignal::extract(const std::vector<uint8_t>& data) const
{
    if (data.empty()) return std::numeric_limits<double>::quiet_NaN();

    // 边界校验：bitLength 必须在 [1, 64] 范围内，否则移位操作是 UB
    if (bitLength == 0 || bitLength > 64) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // ── 1. 提取 raw 值 ──
    uint64_t raw = 0;

    if (isLittleEndian) {
        // Intel 格式：从 startBit 开始，低位在前
        for (uint8_t i = 0; i < bitLength; ++i) {
            uint16_t bitPos = startBit + i;
            uint8_t  byteIdx = bitPos / 8;
            uint8_t  bitIdx  = bitPos % 8;

            if (byteIdx >= data.size()) break;

            if ((data[byteIdx] >> bitIdx) & 1) {
                raw |= (1ULL << i);
            }
        }
    } else {
        // Motorola 格式：大端，跨字节
        uint16_t bitPos = startBit;
        for (uint8_t i = 0; i < bitLength; ++i) {
            uint8_t byteIdx = bitPos / 8;
            uint8_t bitIdx  = bitPos % 8;

            if (byteIdx >= data.size()) break;

            if ((data[byteIdx] >> bitIdx) & 1) {
                raw |= (1ULL << (bitLength - 1 - i));
            }

            // Motorola 字节内从高到低，到字节边界跳到下一字节高位
            if (bitIdx == 0) {
                bitPos += 15;  // 跳到下一字节的 bit7
            } else {
                bitPos -= 1;
            }
        }
    }

    // ── 2. 处理有符号数 ──
    if (isSigned) {
        uint64_t signBit = 1ULL << (bitLength - 1);
        if (raw & signBit) {
            raw = ~raw & ((1ULL << bitLength) - 1);
            return -(static_cast<double>(raw) + 1) * factor + offset;
        }
    }

    // ── 3. 转换为物理值 ──
    return static_cast<double>(raw) * factor + offset;
}

// ── DbcParser ───────────────────────────────────────────────

DbcParser::DbcParser()
{
    loadBuiltinDatabase();
}

void DbcParser::registerSignal(const DbcSignal& sig)
{
    m_signalToMsg[sig.name] = sig.messageId;
    m_messages[sig.messageId].signals.push_back(sig);
}

void DbcParser::loadBuiltinDatabase()
{
    // ════════════════════════════════════════════════════════
    //  内置 DBC 定义：覆盖引擎、变速箱、ADAS、车身、仪表 ECU
    //  信号布局与 VirtualCan 中的 generate*Frame() 一一对应
    // ════════════════════════════════════════════════════════

    // ── 0x100: Engine ECU ──
    m_messages[0x100] = {0x100, "EngineStatus", 8, "EngineECU", {}};
    registerSignal({"EngineRPM",        0x100, 0,  16, true, false, 0.25, 0,    0, 8000, "rpm",   0});
    registerSignal({"CoolantTemp",      0x100, 16,  8, true, false, 1.0, -40, -40,  215, "°C",   0});
    registerSignal({"FuelLevel",        0x100, 24,  8, true, false, 0.4, 0,     0,  100, "%",    0});
    registerSignal({"OilPressure",       0x100, 32, 16, true, false, 1.0, 0,     0, 1000, "kPa", 0});
    registerSignal({"EngineRunning",     0x100, 48,  1, true, false, 1.0, 0,     0,    1, "",     0});
    registerSignal({"CheckEngine",       0x100, 49,  1, true, false, 1.0, 0,     0,    1, "",     0});

    // ── 0x200: Transmission ECU ──
    m_messages[0x200] = {0x200, "TransmissionStatus", 8, "TransECU", {}};
    registerSignal({"Gear",             0x200, 0,  8, true, true,  1.0, 0,     -1,   8, "",     0});
    registerSignal({"VehicleSpeed",      0x200, 8, 16, true, false, 0.01, 0,    0,  400, "km/h", 0});
    registerSignal({"TransmissionTemp",  0x200, 24, 8, true, false, 1.0, -40, -40, 215, "°C",   0});
    registerSignal({"UpShiftIndicator",  0x200, 32, 1, true, false, 1.0, 0,     0,    1, "",     0});
    registerSignal({"DownShiftIndicator",0x200, 33, 1, true, false, 1.0, 0,     0,    1, "",     0});

    // ── 0x300: ADAS ECU ──
    m_messages[0x300] = {0x300, "AdasStatus", 8, "AdasECU", {}};
    registerSignal({"FCW_Active",        0x300, 0, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"LDW_Active",        0x300, 1, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"AEB_Active",        0x300, 2, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"BSM_Active",        0x300, 3, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"FCW_Level",         0x300, 8, 8, true, false, 1.0, 0, 0, 3, "", 0});
    registerSignal({"LDW_Direction",     0x300, 16, 8, true, false, 1.0, 0, 0, 2, "", 0});
    registerSignal({"DistanceToVehicle", 0x300, 24,16, true, false, 1.0, 0, 0, 65535, "cm", 0});
    registerSignal({"SpeedLimit",        0x300, 40, 8, true, false, 1.0, 0, 0, 255, "km/h", 0});

    // ── 0x400: Body ECU ──
    m_messages[0x400] = {0x400, "BodyStatus", 8, "BodyECU", {}};
    registerSignal({"LeftTurnSignal",    0x400, 0, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"RightTurnSignal",   0x400, 1, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"HighBeam",          0x400, 2, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"Handbrake",         0x400, 3, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"DoorOpen",          0x400, 4, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"SeatbeltUnfastened",0x400, 5, 1, true, false, 1.0, 0, 0, 1, "", 0});
    registerSignal({"AmbientLight",     0x400, 8, 8, true, false, 1.0, 0, 0, 255, "lux", 0});
    registerSignal({"InteriorTemp",     0x400, 16,8, true, false, 1.0, -40, -40, 215, "°C", 0});

    // ── 0x500: Cluster ECU ──
    m_messages[0x500] = {0x500, "ClusterInfo", 8, "Cluster", {}};
    registerSignal({"Odometer",          0x500, 0, 32, true, false, 1.0, 0, 0, 4294967295, "km", 0});
    registerSignal({"TripA",             0x500, 32,16, true, false, 0.1, 0, 0, 6553.5, "km", 0});
    registerSignal({"AmbientTemp",       0x500, 48, 8, true, false, 1.0, -40, -40, 215, "°C", 0});

    std::cout << "[DbcParser] Built-in database loaded: "
              << m_messages.size() << " messages, "
              << m_signalToMsg.size() << " signals\n";
}

// ── .dbc 文件解析（简化版）─────────────────────────────────

bool DbcParser::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[DbcParser] Cannot open DBC file: " << path << "\n";
        return false;
    }

    std::string line;
    DbcMessage* currentMsg = nullptr;
    DbcSignal*  currentSig = nullptr;

    while (std::getline(file, line)) {
        // 去除空白
        auto trim = [](std::string s) {
            size_t a = s.find_first_not_of(" \t\r\n");
            if (a == std::string::npos) return std::string{};
            size_t b = s.find_last_not_of(" \t\r\n");
            return s.substr(a, b - a + 1);
        };
        line = trim(line);
        if (line.empty()) continue;

        // BO_ <id>: <name> <sender>
        if (line.rfind("BO_ ", 0) == 0) {
            std::istringstream iss(line.substr(4));
            uint32_t id;
            char colon;
            std::string name, sender;
            iss >> std::hex >> id >> colon >> name >> sender;
            // 去除 name 末尾 ':'
            if (!name.empty() && name.back() == ':') name.pop_back();
            m_messages[id] = {id, name, 0, sender, {}};
            currentMsg = &m_messages[id];
        }
        // SG_ <name> : <start>|<length>@<endian><sign> (<factor>,<offset>) [<min>|<max>] "<unit>" <receiver>
        else if (line.rfind(" SG_ ", 0) == 0 && currentMsg) {
            // 简化解析，处理 Intel 格式
            DbcSignal sig;
            sig.messageId = currentMsg->id;

            // 手动解析关键部分
            size_t colon = line.find(':');
            if (colon == std::string::npos) continue;

            // 信号名
            std::string namePart = trim(line.substr(5, colon - 5));
            sig.name = namePart;

            // 格式: start|length@ endian sign (factor,offset) [min|max] "unit" receiver
            std::string rest = trim(line.substr(colon + 1));
            // 提取 start|length@...
            size_t at = rest.find('@');
            if (at == std::string::npos) continue;

            std::string startLen = rest.substr(0, at);
            size_t bar = startLen.find('|');
            sig.startBit = static_cast<uint8_t>(std::stoi(startLen.substr(0, bar)));
            sig.bitLength = static_cast<uint8_t>(std::stoi(startLen.substr(bar + 1)));

            // endian + sign
            char endian = rest[at + 1];
            char sign = rest[at + 2];
            sig.isLittleEndian = (endian == '1');
            sig.isSigned = (sign == '-');

            // factor, offset
            size_t lp = rest.find('(');
            size_t rp = rest.find(')');
            if (lp != std::string::npos && rp != std::string::npos) {
                std::string fo = rest.substr(lp + 1, rp - lp - 1);
                size_t comma = fo.find(',');
                sig.factor = std::stod(trim(fo.substr(0, comma)));
                sig.offset = std::stod(trim(fo.substr(comma + 1)));
            }

            // unit
            size_t q1 = rest.find('"');
            size_t q2 = rest.find('"', q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos) {
                sig.unit = rest.substr(q1 + 1, q2 - q1 - 1);
            }

            registerSignal(sig);
            currentSig = &m_messages[currentMsg->id].signals.back();
        }
    }

    std::cout << "[DbcParser] DBC file loaded: " << path
              << " (" << m_messages.size() << " messages, "
              << m_signalToMsg.size() << " signals)\n";
    return true;
}

// ── 解析一帧 ────────────────────────────────────────────────

std::unordered_map<std::string, double>
DbcParser::parseFrame(const CanFrame& frame) const
{
    std::unordered_map<std::string, double> result;

    auto it = m_messages.find(frame.id);
    if (it == m_messages.end()) return result;

    for (const auto& sig : it->second.signals) {
        result[sig.name] = sig.extract(frame.data);
    }

    return result;
}

double DbcParser::getSignal(const CanFrame& frame,
                            const std::string& signalName) const
{
    auto it = m_messages.find(frame.id);
    if (it == m_messages.end()) return std::numeric_limits<double>::quiet_NaN();

    for (const auto& sig : it->second.signals) {
        if (sig.name == signalName) {
            return sig.extract(frame.data);
        }
    }
    return std::numeric_limits<double>::quiet_NaN();
}

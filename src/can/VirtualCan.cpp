#include "VirtualCan.h"
#include <chrono>
#include <iostream>
#include <cmath>

VirtualCan::VirtualCan(int seed) : m_rng(seed) {}

VirtualCan::~VirtualCan()
{
    close();
}

bool VirtualCan::open(const std::string& channel)
{
    if (m_open.load()) return true;
    m_open.store(true);

    m_rxRunning.store(true);
    m_rxThread = std::thread(&VirtualCan::rxThreadFunc, this);

    std::cout << "[VirtualCan] Channel '" << channel << "' opened\n";
    return true;
}

void VirtualCan::close()
{
    stopAutoSimulation();

    m_open.store(false);
    m_rxRunning.store(false);
    m_queueCv.notify_all();

    if (m_rxThread.joinable()) m_rxThread.join();
}

bool VirtualCan::send(const CanFrame& frame)
{
    // 发送帧直接回环到接收队列（loopback 模式）
    injectFrame(frame);
    return true;
}

void VirtualCan::injectFrame(const CanFrame& frame)
{
    {
        std::lock_guard<std::mutex> lk(m_queueMutex);
        if (m_frameQueue.size() > 1024) {
            m_frameQueue.pop();
        }
        m_frameQueue.push(frame);
    }
    m_queueCv.notify_one();
}

void VirtualCan::rxThreadFunc()
{
    while (m_rxRunning.load())
    {
        CanFrame frame;
        {
            std::unique_lock<std::mutex> lk(m_queueMutex);
            m_queueCv.wait_for(lk, std::chrono::milliseconds(100),
                [this] { return !m_frameQueue.empty() || !m_rxRunning.load(); });

            if (!m_rxRunning.load()) return;
            if (m_frameQueue.empty()) continue;

            frame = m_frameQueue.front();
            m_frameQueue.pop();
        }

        frame.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
        notifyFrame(frame);
    }
}

// ── 自动模拟 ─────────────────────────────────────────────────

void VirtualCan::startAutoSimulation()
{
    if (m_simRunning.load()) return;
    m_simRunning.store(true);
    m_simThread = std::thread(&VirtualCan::simThreadFunc, this);
    std::cout << "[VirtualCan] Auto simulation started\n";
}

void VirtualCan::stopAutoSimulation()
{
    m_simRunning.store(false);
    if (m_simThread.joinable()) m_simThread.join();
}

void VirtualCan::simThreadFunc()
{
    using namespace std::chrono;

    while (m_simRunning.load())
    {
        ++m_simTickCount;

        // 每 tick 更新模拟物理值
        // 加速/减速循环模拟：0 → 220 km/h 再回到 0
        int cycle = m_simTickCount % 800;  // 800 ticks ≈ 40s @ 50ms
        if (cycle < 300) {
            // 加速阶段
            double t = static_cast<double>(cycle) / 300.0;
            m_simSpeed = 220.0 * (1.0 - std::cos(t * M_PI)) * 0.5;
        } else if (cycle < 500) {
            // 巡航阶段
            m_simSpeed = 220.0 + std::sin(cycle * 0.1) * 2.0;
        } else {
            // 减速阶段
            double t = static_cast<double>(cycle - 500) / 300.0;
            m_simSpeed = 220.0 * (1.0 + std::cos(t * M_PI)) * 0.5;
        }

        // RPM 与车速关联：模拟换挡
        m_simGear = static_cast<int>(m_simSpeed / 30.0);
        if (m_simGear > 8) m_simGear = 8;
        if (m_simSpeed < 1.0) m_simGear = 0;
        double gearRatio = (m_simGear > 0) ? 30.0 * m_simGear : 10.0;
        m_simRpm = (m_simSpeed / gearRatio) * 6000.0;
        if (m_simRpm > 7500) m_simRpm = 7500;
        if (m_simRpm < 800)  m_simRpm = 800 + std::sin(m_simTickCount * 0.3) * 100;

        // 冷却液温度缓慢上升
        m_simCoolant = 40.0 + std::min(50.0, m_simTickCount * 0.02);
        if (m_simCoolant > 105.0) m_simCoolant = 105.0;

        // 油量缓慢下降
        if (m_simTickCount % 200 == 0) m_simFuel -= 1.0;
        if (m_simFuel < 0) m_simFuel = 100.0;

        // 转向灯间歇闪烁
        m_simLeftTurn  = (cycle > 100 && cycle < 150);
        m_simRightTurn = (cycle > 450 && cycle < 500);

        // 远光灯
        m_simHighBeam = (cycle > 600 && cycle < 650);

        // 手刹：低速时拉起
        m_simHandbrake = (m_simSpeed < 5.0);

        // 车门
        m_simDoorOpen = (cycle == 0 || cycle == 400);

        // 里程累加
        m_simOdometer += static_cast<int>(m_simSpeed * 0.014);  // 粗略模拟

        // ADAS 状态
        if (m_simSpeed > 120)       m_simAdasState = 1;  // 前车碰撞预警
        else if (m_simSpeed > 60)   m_simAdasState = 2;  // 车道偏离
        else                        m_simAdasState = 0;  // 正常

        // 发送各 ECU 帧
        injectFrame(generateEngineFrame());
        injectFrame(generateTransmissionFrame());
        injectFrame(generateAdasFrame());
        injectFrame(generateBodyFrame());
        injectFrame(generateClusterFrame());

        std::this_thread::sleep_for(
            std::chrono::milliseconds(m_simIntervalMs));
    }
}

// ── 模拟 ECU 报文生成 ───────────────────────────────────────
// 使用小端字节序（Intel 格式），与大多数汽车 ECU 一致

CanFrame VirtualCan::generateEngineFrame()
{
    // ID 0x100: Engine ECU
    // Signal layout (little-endian):
    //   Byte 0-1: RPM         (raw = rpm * 4,     16 bit)
    //   Byte 2:   CoolantTemp (raw = temp + 40,   8 bit)
    //   Byte 3:   FuelLevel  (raw = percent,      8 bit)
    //   Byte 4-5: OilPressure (raw = kPa,        16 bit)
    //   Byte 6:   EngineStatus (bit0=running, bit1=checkEngine)
    //   Byte 7:   reserved

    CanFrame f;
    f.id = 0x100;
    f.dlc = 8;
    f.data.resize(8, 0);

    uint16_t rpmRaw = static_cast<uint16_t>(m_simRpm * 4.0);
    f.data[0] = rpmRaw & 0xFF;
    f.data[1] = (rpmRaw >> 8) & 0xFF;

    f.data[2] = static_cast<uint8_t>(m_simCoolant + 40);
    f.data[3] = static_cast<uint8_t>(m_simFuel);

    uint16_t oilRaw = static_cast<uint16_t>(300 + std::sin(m_simTickCount * 0.1) * 50);
    f.data[4] = oilRaw & 0xFF;
    f.data[5] = (oilRaw >> 8) & 0xFF;

    f.data[6] = 0x01;  // engine running

    return f;
}

CanFrame VirtualCan::generateTransmissionFrame()
{
    // ID 0x200: Transmission ECU
    //   Byte 0: Gear (-1=R, 0=N/P, 1-8=forward)
    //   Byte 1-2: VehicleSpeed (raw = speed * 100, 16 bit)
    //   Byte 3: TransmissionTemp
    //   Byte 4: ShiftIndicator (bit0=up, bit1=down)
    //   Byte 5-7: reserved

    CanFrame f;
    f.id = 0x200;
    f.dlc = 8;
    f.data.resize(8, 0);

    f.data[0] = static_cast<uint8_t>(m_simGear);

    uint16_t speedRaw = static_cast<uint16_t>(m_simSpeed * 100.0);
    f.data[1] = speedRaw & 0xFF;
    f.data[2] = (speedRaw >> 8) & 0xFF;

    f.data[3] = static_cast<uint8_t>(m_simCoolant - 10);

    // 换挡提示
    if (m_simRpm > 6000) f.data[4] = 0x01;  // suggest upshift

    return f;
}

CanFrame VirtualCan::generateAdasFrame()
{
    // ID 0x300: ADAS ECU
    //   Byte 0: AdasStatus (bit0=FCW, bit1=LDW, bit2=AEB, bit3=BSM)
    //   Byte 1: FCW_Level (0-3)
    //   Byte 2: LDW_Direction (0=none, 1=left, 2=right)
    //   Byte 3-4: DistanceToVehicle (cm, 16 bit)
    //   Byte 5: SpeedLimit (km/h, 0=none)
    //   Byte 6-7: reserved

    CanFrame f;
    f.id = 0x300;
    f.dlc = 8;
    f.data.resize(8, 0);

    uint8_t adasStatus = 0;
    uint8_t fcwLevel = 0;
    uint8_t ldwDir = 0;
    uint16_t dist = 0;

    if (m_simAdasState == 1) {       // FCW
        adasStatus |= 0x01;
        fcwLevel = (m_simSpeed > 180) ? 3 : 2;
        dist = static_cast<uint16_t>(1500 - m_simSpeed * 5);
    } else if (m_simAdasState == 2) { // LDW
        adasStatus |= 0x02;
        ldwDir = (m_simTickCount % 20 < 10) ? 1 : 2;
        dist = 4000;
    } else {
        dist = 8000;
    }

    f.data[0] = adasStatus;
    f.data[1] = fcwLevel;
    f.data[2] = ldwDir;
    f.data[3] = dist & 0xFF;
    f.data[4] = (dist >> 8) & 0xFF;
    f.data[5] = (m_simSpeed > 100) ? 120 : 0;  // 限速

    return f;
}

CanFrame VirtualCan::generateBodyFrame()
{
    // ID 0x400: Body ECU
    //   Byte 0 bit0: LeftTurn
    //   Byte 0 bit1: RightTurn
    //   Byte 0 bit2: HighBeam
    //   Byte 0 bit3: Handbrake
    //   Byte 0 bit4: DoorOpen
    //   Byte 0 bit5: SeatbeltUnfastened
    //   Byte 1: AmbientLight (lux, 8 bit)
    //   Byte 2: InteriorTemp (raw = temp + 40)
    //   Byte 3-7: reserved

    CanFrame f;
    f.id = 0x400;
    f.dlc = 8;
    f.data.resize(8, 0);

    uint8_t bodyStatus = 0;
    if (m_simLeftTurn)  bodyStatus |= 0x01;
    if (m_simRightTurn) bodyStatus |= 0x02;
    if (m_simHighBeam)  bodyStatus |= 0x04;
    if (m_simHandbrake) bodyStatus |= 0x08;
    if (m_simDoorOpen)  bodyStatus |= 0x10;
    bodyStatus |= 0x20;  // seatbelt unfastened at sim start

    f.data[0] = bodyStatus;
    f.data[1] = static_cast<uint8_t>(200 + std::sin(m_simTickCount * 0.05) * 50);
    f.data[2] = static_cast<uint8_t>(22 + 40);  // 22°C

    return f;
}

CanFrame VirtualCan::generateClusterFrame()
{
    // ID 0x500: Cluster ECU（里程等仪表自身信息）
    //   Byte 0-3: Odometer (km, 32 bit)
    //   Byte 4-5: TripA (100m, 16 bit)
    //   Byte 6: AmbientTemp (raw = temp + 40)
    //   Byte 7: reserved

    CanFrame f;
    f.id = 0x500;
    f.dlc = 8;
    f.data.resize(8, 0);

    uint32_t odo = static_cast<uint32_t>(m_simOdometer);
    f.data[0] = odo & 0xFF;
    f.data[1] = (odo >> 8) & 0xFF;
    f.data[2] = (odo >> 16) & 0xFF;
    f.data[3] = (odo >> 24) & 0xFF;

    uint16_t trip = static_cast<uint16_t>(m_simTickCount * 5);
    f.data[4] = trip & 0xFF;
    f.data[5] = (trip >> 8) & 0xFF;

    f.data[6] = static_cast<uint8_t>(18 + 40);  // 18°C ambient

    return f;
}

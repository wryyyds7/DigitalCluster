#include "VehicleModel.h"
#include "can/VirtualCan.h"
#include <iostream>
#include <cmath>

VehicleModel::VehicleModel(QObject* parent)
    : QObject(parent)
{
    // ECU 名称映射
    m_ecuNames[0x100] = "EngineECU";
    m_ecuNames[0x200] = "TransmissionECU";
    m_ecuNames[0x300] = "AdasECU";
    m_ecuNames[0x400] = "BodyECU";
    m_ecuNames[0x500] = "ClusterECU";

    // 超时检测：每 500ms 检查一次
    m_timeoutTimer.setInterval(500);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &VehicleModel::onTimeoutCheck);

    // 电源状态机信号
    m_powerSM.stateChanged.connect(
        [this](PowerState s) { onPowerStateChanged(s); });
}

VehicleModel::~VehicleModel()
{
    if (m_can) m_can->close();
}

bool VehicleModel::initialize(const std::string& canChannel, bool autoSim)
{
    // 默认创建 VirtualCAN
    if (!m_can) {
        m_can = std::make_unique<VirtualCan>(42);
    }

    // 注册帧回调
    m_can->setFrameCallback(
        [this](const CanFrame& frame) {
            // 跨线程投递到 Qt 事件循环
            QMetaObject::invokeMethod(this,
                [this, frame] { onCanFrame(frame); },
                Qt::QueuedConnection);
        });

    if (!m_can->open(canChannel)) {
        emit canError("Failed to open CAN channel: " + QString::fromStdString(canChannel));
        return false;
    }

    // 如果是 VirtualCAN 且 autoSim 为 true，启动自动模拟
    if (autoSim) {
        if (auto* vcan = dynamic_cast<VirtualCan*>(m_can.get())) {
            vcan->startAutoSimulation();
        }
    }

    m_timeoutTimer.start();
    std::cout << "[VehicleModel] Initialized on channel '" << canChannel << "'\n";
    return true;
}

void VehicleModel::setCanInterface(std::unique_ptr<CanInterface> iface)
{
    if (m_can) m_can->close();
    m_can = std::move(iface);
}

void VehicleModel::onCanFrame(const CanFrame& frame)
{
    // 记录接收时间
    m_lastRxTime[frame.id] = std::chrono::steady_clock::now();

    // DBC 解析
    auto signals = m_dbc.parseFrame(frame);
    if (signals.empty()) return;

    // 更新数据
    bool speedChanged = false;
    bool rpmChanged   = false;
    bool gearChanged  = false;
    bool fuelChanged  = false;
    bool tempChanged  = false;

    for (const auto& [name, value] : signals) {
        updateSignal(name, value);

        if (name == "VehicleSpeed")    speedChanged = true;
        if (name == "EngineRPM")        rpmChanged = true;
        if (name == "Gear")             gearChanged = true;
        if (name == "FuelLevel")        fuelChanged = true;
        if (name == "CoolantTemp")      tempChanged = true;
    }

    // 发送变更信号
    emit dataChanged(m_data);

    if (speedChanged) emit speedChanged(m_data.speed);
    if (rpmChanged)   emit rpmChanged(m_data.rpm);
    if (gearChanged)  emit gearChanged(m_data.gear);
    if (fuelChanged)  emit fuelChanged(m_data.fuelLevel);
    if (tempChanged)  emit temperatureChanged(m_data.coolantTemp);

    // ADAS 告警检测
    checkAdasWarnings();
}

void VehicleModel::updateSignal(const std::string& name, double value)
{
    if      (name == "VehicleSpeed")       m_data.speed = value;
    else if (name == "EngineRPM")           m_data.rpm = value;
    else if (name == "Gear")               m_data.gear = static_cast<int>(value);
    else if (name == "CoolantTemp")        m_data.coolantTemp = value;
    else if (name == "OilPressure")        m_data.oilPressure = value;
    else if (name == "FuelLevel")          m_data.fuelLevel = value;
    else if (name == "EngineRunning")      m_data.engineRunning = (value > 0.5);
    else if (name == "CheckEngine")        m_data.checkEngine = (value > 0.5);
    else if (name == "UpShiftIndicator")   m_data.upShift = (value > 0.5);
    else if (name == "DownShiftIndicator") m_data.downShift = (value > 0.5);
    else if (name == "TransmissionTemp")   m_data.transTemp = value;
    else if (name == "LeftTurnSignal")      m_data.leftTurn = (value > 0.5);
    else if (name == "RightTurnSignal")     m_data.rightTurn = (value > 0.5);
    else if (name == "HighBeam")            m_data.highBeam = (value > 0.5);
    else if (name == "Handbrake")           m_data.handbrake = (value > 0.5);
    else if (name == "DoorOpen")            m_data.doorOpen = (value > 0.5);
    else if (name == "SeatbeltUnfastened")  m_data.seatbeltUnfastened = (value > 0.5);
    else if (name == "AmbientLight")        m_data.ambientLight = value;
    else if (name == "InteriorTemp")        m_data.interiorTemp = value;
    else if (name == "FCW_Active")          m_data.fcwActive = (value > 0.5);
    else if (name == "LDW_Active")         m_data.ldwActive = (value > 0.5);
    else if (name == "AEB_Active")         m_data.aebActive = (value > 0.5);
    else if (name == "BSM_Active")          m_data.bsmActive = (value > 0.5);
    else if (name == "FCW_Level")          m_data.fcwLevel = static_cast<int>(value);
    else if (name == "LDW_Direction")      m_data.ldwDirection = static_cast<int>(value);
    else if (name == "DistanceToVehicle")  m_data.distanceToVehicle = value;
    else if (name == "SpeedLimit")         m_data.speedLimit = static_cast<int>(value);
    else if (name == "Odometer")           m_data.odometer = static_cast<uint32_t>(value);
    else if (name == "TripA")              m_data.tripA = value;
    else if (name == "AmbientTemp")        m_data.ambientTemp = value;
}

void VehicleModel::checkAdasWarnings()
{
    if (m_data.fcwActive) {
        QString msg;
        if (m_data.fcwLevel >= 3)      msg = "FRONT COLLISION WARNING: CRITICAL";
        else if (m_data.fcwLevel >= 2) msg = "Front Collision Warning";
        else                            msg = "Front Vehicle Detected";
        emit adasWarningChanged(0, m_data.fcwLevel, msg);
    }
    if (m_data.ldwActive) {
        QString dir = (m_data.ldwDirection == 1) ? " (Left)" : " (Right)";
        emit adasWarningChanged(1, 1, "Lane Departure" + dir);
    }
    if (m_data.aebActive) {
        emit adasWarningChanged(2, 3, "AUTOMATIC EMERGENCY BRAKING");
    }
}

void VehicleModel::onTimeoutCheck()
{
    auto now = std::chrono::steady_clock::now();
    constexpr int TIMEOUT_MS = 2000;  // 2 秒无报文视为超时

    for (const auto& [id, lastTime] : m_lastRxTime) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastTime).count();
        if (elapsed > TIMEOUT_MS) {
            auto nameIt = m_ecuNames.find(id);
            QString ecuName = (nameIt != m_ecuNames.end())
                ? QString::fromStdString(nameIt->second)
                : QString("ECU 0x%1").arg(id, 0, 16);
            emit signalTimeout(ecuName);
        }
    }
}

void VehicleModel::onPowerStateChanged(PowerState state)
{
    emit powerStateChanged(state);
}

void VehicleModel::requestPowerOn()
{
    m_powerSM.transition(PowerEvent::KEY_ACC);
}

void VehicleModel::requestPowerOff()
{
    // 根据当前状态选择合适的关闭路径
    auto state = m_powerSM.currentState();
    if (state == PowerState::CRANK || state == PowerState::START) {
        // 启动中强制关机：先回到 ON 再关
        m_powerSM.transition(PowerEvent::ENGINE_STARTED);
    }
    m_powerSM.transition(PowerEvent::KEY_OFF);
}

void VehicleModel::requestStartEngine()
{
    m_powerSM.transition(PowerEvent::KEY_START);
}

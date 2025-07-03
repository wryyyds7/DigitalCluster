#pragma once

#include "VehicleData.h"
#include "can/CanInterface.h"
#include "can/DbcParser.h"
#include "core/PowerStateMachine.h"
#include <QObject>
#include <QTimer>
#include <memory>
#include <functional>
#include <chrono>
#include <unordered_map>

/**
 * @brief 车辆数据 Model（MVVM Model 层）。
 *
 * 职责：
 *   1. 持有 CAN 接口与 DBC 解析器
 *   2. 接收原始 CAN 帧 → DBC 解析 → 更新 VehicleData
 *   3. 信号超时检测（某 ECU 通信丢失时发出告警）
 *   4. 管理上下电状态机
 *   5. 通过 signalDataChanged() 通知 ViewModel 刷新
 *
 * 严禁在此层操作任何 UI / QML 代码。
 */
class VehicleModel : public QObject
{
    Q_OBJECT

public:
    explicit VehicleModel(QObject* parent = nullptr);
    ~VehicleModel() override;

    /// 初始化 CAN 接口（默认使用 VirtualCAN）
    /// autoSim: 是否启动 VirtualCAN 自动模拟（对非 VirtualCAN 后端无效）
    bool initialize(const std::string& canChannel = "virtual", bool autoSim = true);

    /// 获取当前车辆数据（const 引用，线程安全读取由调用方保证）
    const VehicleData& data() const { return m_data; }

    /// 获取 DBC 解析器
    const DbcParser& dbc() const { return m_dbc; }

    /// 获取电源状态机
    PowerStateMachine& powerStateMachine() { return m_powerSM; }
    const PowerStateMachine& powerStateMachine() const { return m_powerSM; }

    /// 设置 CAN 接口（用于注入 mock 接口做单元测试）
    void setCanInterface(std::unique_ptr<CanInterface> iface);

    /// 手动触发电源状态转换
    void requestPowerOn();
    void requestPowerOff();
    void requestStartEngine();

signals:
    /// 车辆数据变更信号（ViewModel 连接此信号刷新 Q_PROPERTY）
    void dataChanged(const VehicleData& data);

    /// 特定信号变更信号
    void speedChanged(double speed);
    void rpmChanged(double rpm);
    void gearChanged(int gear);
    void fuelChanged(double fuel);
    void temperatureChanged(double temp);
    void adasWarningChanged(int type, int level, const QString& message);

    /// 电源状态变更信号
    void powerStateChanged(PowerState state);

    /// 信号超时告警
    void signalTimeout(const QString& ecuName);

    /// CAN 接口错误
    void canError(const QString& error);

private slots:
    /// CAN 帧到达处理
    void onCanFrame(const CanFrame& frame);

    /// 超时检测定时器
    void onTimeoutCheck();

    /// 电源状态变更
    void onPowerStateChanged(PowerState state);

private:
    void updateSignal(const std::string& name, double value);
    void checkAdasWarnings();

private:
    VehicleData                     m_data;
    DbcParser                       m_dbc;
    PowerStateMachine               m_powerSM;
    std::unique_ptr<CanInterface>   m_can;

    /// 各 ECU 最后接收时间（用于超时检测）
    std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> m_lastRxTime;

    /// 超时检测定时器
    QTimer                          m_timeoutTimer;

    /// ECU 名称映射（用于超时告警显示）
    std::unordered_map<uint32_t, std::string> m_ecuNames;
};

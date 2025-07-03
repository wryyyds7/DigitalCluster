#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

/**
 * @brief 电源状态枚举
 *
 * 完整的车规级上下电状态序列：
 *   OFF → ACC → ON → START → CRANK → ON（引擎启动后回到 ON）
 *
 * 各状态含义：
 *   OFF:   整车断电，仪表全灭
 *   ACC:   附件供电，部分信息显示（时钟、多媒体）
 *   ON:    全系统供电，仪表自检（扫针动画）
 *   START: 启动请求
 *   CRANK: 发动机正在启动（启动机拖转），仪表显示"Starting..."
 */
enum class PowerState
{
    OFF   = 0,
    ACC   = 1,
    ON    = 2,
    START = 3,
    CRANK = 4
};

/// 电源事件
enum class PowerEvent
{
    KEY_OFF,      ///< 钥匙拧到 OFF
    KEY_ACC,      ///< 钥匙拧到 ACC
    KEY_ON,       ///< 钥匙拧到 ON
    KEY_START,    ///< 钥匙拧到 START
    ENGINE_STARTED, ///< 发动机启动成功
    CRANK_TIMEOUT,  ///< 拖转超时
    CRANK_FAIL     ///< 拖转失败
};

/**
 * @brief 上下电状态机
 *
 * 使用状态转移表实现，支持信号/回调通知状态变化。
 * 线程安全：内部使用互斥锁保护。
 */
class PowerStateMachine
{
public:
    using StateCallback = std::function<void(PowerState)>;

    PowerStateMachine();

    /// 执行状态转移
    /// 返回 true 表示转移成功
    bool transition(PowerEvent event);

    /// 获取当前状态
    PowerState currentState() const;

    /// 获取状态名称（用于 UI 显示）
    static std::string stateName(PowerState s);
    static std::string stateDescription(PowerState s);

    /// 注册状态变更回调
    void onStateChanged(StateCallback cb) { stateChanged = std::move(cb); }

    /// 状态变更信号（函数对象，外部连接）
    StateCallback stateChanged;

    /// 获取当前状态允许转移的事件列表
    std::vector<PowerEvent> allowedEvents() const;

private:
    /// 初始化状态转移表
    void initTransitions();

    struct Transition {
        PowerState from;
        PowerEvent event;
        PowerState to;
    };

    std::vector<Transition>          m_transitions;
    mutable std::mutex              m_mutex;
    PowerState                      m_currentState;
};

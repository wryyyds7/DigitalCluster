#pragma once

#include "CanInterface.h"
#include <unordered_map>
#include <queue>
#include <random>

/**
 * @brief 虚拟 CAN 后端。
 *
 * 不依赖物理 CAN 硬件，内部使用生产者-消费者队列模拟 ECU 发报。
 * 支持两种模式：
 *   1. 外部注入：通过 injectFrame() 直接灌入帧（测试场景）
 *   2. 自动模拟：内置 ECU 模拟器，按 DBC 定义的周期自动生成信号值
 *
 * 跨平台可用，是默认开发/测试后端。
 */
class VirtualCan : public CanInterface
{
public:
    explicit VirtualCan(int seed = 42);
    ~VirtualCan() override;

    bool open(const std::string& channel) override;
    void close() override;
    bool send(const CanFrame& frame) override;

    /// 外部注入一帧（供模拟器/测试调用）
    void injectFrame(const CanFrame& frame);

    /// 启动自动模拟模式：内置 ECU 模拟器周期性发报
    void startAutoSimulation();
    void stopAutoSimulation();

    /// 设置自动模拟周期（毫秒），默认 50ms (20 fps)
    void setSimulationIntervalMs(int ms) { m_simIntervalMs = ms; }

private:
    /// 接收线程入口
    void rxThreadFunc();

    /// 自动模拟线程入口
    void simThreadFunc();

    /// 生成模拟 CAN 帧（引擎 ECU、变速箱 ECU、ADAS ECU 等）
    CanFrame generateEngineFrame();
    CanFrame generateTransmissionFrame();
    CanFrame generateAdasFrame();
    CanFrame generateBodyFrame();
    CanFrame generateClusterFrame();

    // ── 接收线程 ──
    std::thread             m_rxThread;
    std::atomic<bool>       m_rxRunning{false};

    // ── 帧队列 ──
    std::mutex              m_queueMutex;
    std::condition_variable m_queueCv;
    std::queue<CanFrame>    m_frameQueue;

    // ── 自动模拟线程 ──
    std::thread             m_simThread;
    std::atomic<bool>       m_simRunning{false};
    int                     m_simIntervalMs = 50;

    // ── 模拟状态 ──
    std::mt19937            m_rng;
    double                  m_simSpeed      = 0.0;   ///< km/h
    double                  m_simRpm        = 0.0;
    double                  m_simCoolant    = 40.0;   ///< °C
    double                  m_simFuel       = 75.0;   ///< %
    int                     m_simGear       = 0;      ///< -1=R, 0=N, 1-8
    int                     m_simOdometer   = 0;
    bool                    m_simLeftTurn   = false;
    bool                    m_simRightTurn  = false;
    bool                    m_simHighBeam    = false;
    bool                    m_simHandbrake   = true;
    bool                    m_simDoorOpen    = false;
    int                     m_simAdasState   = 0;
    int                     m_simTickCount   = 0;
};

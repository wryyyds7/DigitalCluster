#pragma once

#include "CanFrame.h"
#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

/**
 * @brief CAN 接口抽象基类。
 *
 * 统一封装 SocketCAN (Linux)、PCAN (Windows)、VirtualCAN 等后端，
 * 上层代码只需依赖此接口，实现平台无关的 CAN 读写。
 *
 * 线程安全：setFrameCallback() 和 notifyFrame() 通过 m_callbackMutex
 * 互斥保护，确保回调可在 rx 线程启动后安全设置。
 */
class CanInterface
{
public:
    /// 帧接收回调类型
    using FrameCallback = std::function<void(const CanFrame&)>;

    virtual ~CanInterface() = default;

    /// 打开 CAN 设备/通道
    virtual bool open(const std::string& channel) = 0;

    /// 关闭接口
    virtual void close() = 0;

    /// 发送一帧
    virtual bool send(const CanFrame& frame) = 0;

    /// 注册帧接收回调（线程安全，可在 open 前后调用）
    void setFrameCallback(FrameCallback cb)
    {
        std::lock_guard<std::mutex> lk(m_callbackMutex);
        m_callback = std::move(cb);
    }

    /// 是否已打开
    bool isOpen() const { return m_open.load(); }

protected:
    FrameCallback      m_callback;
    std::mutex         m_callbackMutex;
    std::atomic<bool>  m_open{false};

    /// 子类调用：将接收到的帧投递给回调（线程安全）
    void notifyFrame(const CanFrame& frame)
    {
        FrameCallback cb;
        {
            std::lock_guard<std::mutex> lk(m_callbackMutex);
            cb = m_callback;
        }
        if (cb) cb(frame);
    }
};

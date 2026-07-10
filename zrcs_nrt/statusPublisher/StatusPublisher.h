/// @file StatusPublisher.h
/// @brief 状态发布器 —— 将 RT 轴反馈数据通过 ZMQ PUB 推送给 GUI

#pragma once

#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <vector>

#include <spdlog/spdlog.h>

#include "message.pb.h"
#include "command/RtBridge.h"

/**
 * @brief 状态发布器 (ZMQ PUB)
 * @details 从共享内存读取 RT 轴位置数据，通过 ZMQ PUB socket 发布给 GUI
 *          发布频率: 100Hz (10ms)，数据格式: SystemStatus protobuf
 *
 * 数据来源:
 *  - 轴反馈数据: RtBridge::readLatestAxisFeedback() (RT 共享内存)
 *  - RT 日志:    外部线程通过 enqueueRtLog() 投递
 *
 * 输出:
 *  - ZMQ PUB: tcp://*:5556, SystemStatus protobuf
 */
class StatusPublisher 
{
public:
    /// @brief 构造状态发布器
    /// @param bridge RT 桥接指针，用于读取共享内存中的轴反馈
    explicit StatusPublisher(RtBridge* bridge): context_(1), bridge_(bridge), running_(false) {};
        

    /// @brief 析构，自动停止发布线程
    ~StatusPublisher() { stop(); }

    /// @brief 初始化 ZMQ PUB socket 并绑定端口
    /// @return true 初始化成功；false 绑定失败（端口占用等）
    bool initialize() 
    {
        try
        {
            pub_socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::pub);
            pub_socket_->set(zmq::sockopt::linger, 0);   // 关闭时立即返回，不阻塞等待发送完成
            pub_socket_->set(zmq::sockopt::sndhwm, 100);  // 发送高水位: 100 条，溢出时丢弃旧帧
            pub_socket_->bind(PUB_ENDPOINT);
            spdlog::info("[StatusPublisher] Initialized on {}", PUB_ENDPOINT);
            return true;
        }
        catch (const zmq::error_t& e)
        {
            spdlog::error("[StatusPublisher] Init error: {}", e.what());
            return false;
        }
    }

    /// @brief 启动发布线程（幂等，重复调用安全）
    void start() 
    {
        if (running_.exchange(true)) return;
        pub_thread_ = std::thread(&StatusPublisher::run, this);
        spdlog::info("[StatusPublisher] Started (interval={}ms)", PUB_INTERVAL_MS);
    }

    /// @brief 停止发布线程并关闭 socket（幂等，可在析构外主动调用）
    void stop() 
    {
        const bool wasRunning = running_.exchange(false);
        const bool hasThread = pub_thread_.joinable();
        const bool hasSocket = static_cast<bool>(pub_socket_);
        if (!wasRunning && !hasThread && !hasSocket) return;

        if (pub_thread_.joinable()) 
        {
            pub_thread_.join();
        }
        if (pub_socket_) 
        {
            pub_socket_->close();
            pub_socket_.reset();
        }
        if (wasRunning || hasThread) {
            spdlog::info("[StatusPublisher] Stopped");
        }
    }

    /// @brief 投递一条 RT 日志，将随下一次状态发布一并发送给 GUI
    /// @details 线程安全。队列满时丢弃最旧的一条（FIFO 滑窗）。
    /// @param entry RT 日志条目（时间戳、级别、文件、行号、消息）
    void enqueueRtLog(const zrcs::RtLogEntry& entry) 
    {
        std::lock_guard<std::mutex> lk(log_mutex_);
        if (pending_logs_.size() >= kMaxPendingRtLogs) 
        {
            pending_logs_.pop_front();
        }
        pending_logs_.push_back(entry);
    }

private:
    // ===== 发布配置 =====
    static constexpr const char* PUB_ENDPOINT = "tcp://*:5556";  ///< ZMQ PUB 绑定地址
    static constexpr int PUB_INTERVAL_MS = 10;                   ///< 发布周期 (ms)，对应 100Hz
    static constexpr size_t kMaxPendingRtLogs = 256;             ///< 待发送 RT 日志队列上限

    // ===== ZMQ 资源 =====
    zmq::context_t context_;                       ///< ZMQ 上下文
    std::unique_ptr<zmq::socket_t> pub_socket_;    ///< PUB socket (发布端)

    // ===== 数据源与运行状态 =====
    RtBridge* bridge_;                             ///< RT 桥接（读取共享内存）
    std::thread pub_thread_;                       ///< 发布线程
    std::atomic<bool> running_;                    ///< 运行标志（线程启停控制）

    // ===== RT 日志缓冲 =====
    std::mutex log_mutex_;                         ///< 保护 pending_logs_ 的互斥锁
    std::deque<zrcs::RtLogEntry> pending_logs_;    ///< 待发布的 RT 日志队列

    /// @brief 取出并清空待发送的 RT 日志（线程安全）
    /// @return 本次需要发布的日志向量（按入队顺序）
    std::vector<zrcs::RtLogEntry> drainPendingRtLogs() 
    {
        std::vector<zrcs::RtLogEntry> logs;
        std::lock_guard<std::mutex> lk(log_mutex_);
        logs.reserve(pending_logs_.size());
        while (!pending_logs_.empty()) 
        {
            logs.push_back(pending_logs_.front());
            pending_logs_.pop_front();
        }
        return logs;
    }

    /// @brief 发布线程主循环
    /// @details 每个周期:
    ///          1. 拉取 RT 共享内存中所有新到的轴反馈帧，更新"最新帧"；
    ///          2. 收集待发送的 RT 日志；
    ///          3. 若有反馈或日志，组装 SystemStatus 并通过 PUB 发送；
    ///          4. 休眠一个周期。
    void run()
    {
        zrcs::AxisFeedbackData latestFeedback{};  // 缓存的最新一帧轴反馈
        bool hasFeedback = false;                  // 是否已收到过任何反馈帧

        while (running_) 
        {
            // ---- 1. 拉取所有新到的轴反馈帧 ----
            zrcs::AxisFeedbackData feedback{};
            bool gotFeedback = false;
            while (bridge_->readLatestAxisFeedback(feedback)) 
            {
                gotFeedback = true;
                latestFeedback = feedback;
                hasFeedback = true;
            }

            // ---- 2. 收集待发送的 RT 日志 ----
            auto rtLogs = drainPendingRtLogs();

            // ---- 3. 有数据才组装并发布，避免空包占用带宽 ----
            if (hasFeedback || !rtLogs.empty())
            {
                zrcs_message::SystemStatus status;
                uint8_t count = bridge_->axisCount();

                // 填充各轴反馈（位置/指令位置/指令速度/实际速度/力矩）
                if (hasFeedback) 
                {
                    for (uint8_t i = 0; i < count; ++i) 
                    {
                        auto* axis = status.add_axes();
                        axis->set_axis_id(i);
                        axis->set_position(latestFeedback.position[i]);
                        axis->set_cmd_position(latestFeedback.cmdPosition[i]);
                        axis->set_cmd_velocity(latestFeedback.cmdVelocity[i]);
                        axis->set_velocity(latestFeedback.velocity[i]);
                        axis->set_torque(latestFeedback.torque[i]);
                    }
                }

                // 心跳与丢弃命令计数，GUI 用于判断链路健康度
                status.set_heartbeat(bridge_->heartbeat());
                status.set_dropped_commands(bridge_->droppedCount());

                // 写入 taskSched 状态供 GUI 显示
                auto ts = bridge_->getTaskScheduling();
                switch (ts)
                {
                    case zrcs::TaskScheduling::IDLE:       status.set_system_state("IDLE"); break;
                    case zrcs::TaskScheduling::RUN:         status.set_system_state("RUN"); break;
                    case zrcs::TaskScheduling::STOP:        status.set_system_state("STOP"); break;
                    case zrcs::TaskScheduling::ERROR_STATE: status.set_system_state("ERROR"); break;
                    case zrcs::TaskScheduling::RESET:       status.set_system_state("RESET"); break;
                    case zrcs::TaskScheduling::SHUTDOWN:    status.set_system_state("SHUTDOWN"); break;
                }

                // 附加本周期的 RT 日志
                for (const auto& entry : rtLogs) 
                {
                    auto* log = status.add_rt_logs();
                    log->set_timestamp_us(entry.timestamp_us);
                    log->set_level(entry.level);
                    log->set_file(entry.file);
                    log->set_line(entry.line);
                    log->set_message(entry.message);
                }

                // 序列化并以非阻塞方式发送（dontwait: 缓冲满时直接丢弃，不阻塞发布线程）
                std::string serialized;
                if (status.SerializeToString(&serialized))
                {
                    try {
                           zmq::message_t msg(serialized.size());
                           memcpy(msg.data(), serialized.data(), serialized.size());
                           pub_socket_->send(msg, zmq::send_flags::dontwait);
                        } 
                    catch (const zmq::error_t& e)
                     {
                        if (running_) 
                        {
                            spdlog::warn("[StatusPublisher] Send error: {}", e.what());
                        }
                    }
                }
            }

            // ---- 4. 休眠至下一周期 ----
            std::this_thread::sleep_for(std::chrono::milliseconds(PUB_INTERVAL_MS));
        }
    }
};

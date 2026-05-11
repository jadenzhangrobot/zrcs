#pragma once

#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <fstream>
#include <chrono>
#include <cmath>
#include <deque>
#include <mutex>
#include <vector>

#include <spdlog/spdlog.h>

#include "message.pb.h"
#include "rtBridge/RtBridge.h"
#include "config/Parameter.h"

/**
 * @brief 状态发布器 (ZMQ PUB)
 * @details 从共享内存读取 RT 轴位置数据，通过 ZMQ PUB socket 发布给 GUI
 *          发布频率: 100Hz (10ms)，数据格式: SystemStatus protobuf
 */
class StatusPublisher {
public:
    explicit StatusPublisher(RtBridge* bridge)
        : context_(1), bridge_(bridge), running_(false) {}

    ~StatusPublisher() { stop(); }

    bool initialize() {
        try {
            pub_socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::pub);
            pub_socket_->set(zmq::sockopt::linger, 0);
            pub_socket_->set(zmq::sockopt::sndhwm, 10);
            pub_socket_->bind(PUB_ENDPOINT);
            spdlog::info("[StatusPublisher] Initialized on {}", PUB_ENDPOINT);
            return true;
        } catch (const zmq::error_t& e) {
            spdlog::error("[StatusPublisher] Init error: {}", e.what());
            return false;
        }
    }

    void start() {
        if (running_.exchange(true)) return;
        pub_thread_ = std::thread(&StatusPublisher::run, this);
        spdlog::info("[StatusPublisher] Started (interval={}ms)", PUB_INTERVAL_MS);
    }

    void stop() {
        if (!running_.exchange(false)) return;
        if (pub_thread_.joinable()) {
            pub_thread_.join();
        }
        if (pub_socket_) {
            pub_socket_->close();
            pub_socket_.reset();
        }
        spdlog::info("[StatusPublisher] Stopped");
    }

    void enqueueRtLog(const zrcs::RtLogEntry& entry) {
        std::lock_guard<std::mutex> lk(log_mutex_);
        if (pending_logs_.size() >= kMaxPendingRtLogs) {
            pending_logs_.pop_front();
        }
        pending_logs_.push_back(entry);
    }

private:
    static constexpr const char* PUB_ENDPOINT = "tcp://*:5556";
    static constexpr int PUB_INTERVAL_MS = 10;
    static constexpr size_t kMaxPendingRtLogs = 256;

    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> pub_socket_;
    RtBridge* bridge_;
    std::thread pub_thread_;
    std::atomic<bool> running_;
    std::mutex log_mutex_;
    std::deque<zrcs::RtLogEntry> pending_logs_;

    std::vector<zrcs::RtLogEntry> drainPendingRtLogs() {
        std::vector<zrcs::RtLogEntry> logs;
        std::lock_guard<std::mutex> lk(log_mutex_);
        logs.reserve(pending_logs_.size());
        while (!pending_logs_.empty()) {
            logs.push_back(pending_logs_.front());
            pending_logs_.pop_front();
        }
        return logs;
    }

    void run() {
        zrcs::AxisFeedbackData latestFeedback{};
        bool hasFeedback = false;

        // 轴数据 CSV 日志
        std::ofstream axisLog("axis_log.csv", std::ios::out | std::ios::trunc);
        if (axisLog.is_open()) {
            axisLog << "time_ms,"
                    << "plat_x,plat_y,"
                    << "galvo_x,galvo_y,"
                    << "plat_speed,"
                    << "galvo_speed\n";
        }
        auto startTime = std::chrono::steady_clock::now();

        while (running_) {
            zrcs::AxisFeedbackData feedback{};
            bool gotFeedback = false;
            while (bridge_->readLatestAxisFeedback(feedback)) {
                gotFeedback = true;
                latestFeedback = feedback;
                hasFeedback = true;

                // 写入平台轴（0/1）和振镜轴（2/3）数据到 CSV，逐帧记录避免中间帧丢失
                const uint8_t count = bridge_->axisCount();
                if (axisLog.is_open() && count >= 4) {
                    auto elapsed = std::chrono::steady_clock::now() - startTime;
                    double ms = std::chrono::duration<double, std::milli>(elapsed).count();
                    const double platSpeed = std::hypot(feedback.velocity[0], feedback.velocity[1]);
                    const double galvoSpeed = std::hypot(feedback.velocity[2], feedback.velocity[3]);
                    axisLog << ms << ","
                            // 平台实际位置
                            << feedback.position[0] << ","
                            << feedback.position[1] << ","
                            // 振镜实际位置
                            << feedback.position[2] << ","
                            << feedback.position[3] << ","
                            // 平台/振镜合速度
                            << platSpeed << ","
                            << galvoSpeed << "\n";
                }
            }

            auto rtLogs = drainPendingRtLogs();
            if (hasFeedback || !rtLogs.empty()) {
                zrcs_message::SystemStatus status;
                uint8_t count = bridge_->axisCount();
                if (count == 0) count = static_cast<uint8_t>(zrcs::kAxisMax);

                if (hasFeedback) {
                    for (uint8_t i = 0; i < count; ++i) {
                        auto* axis = status.add_axes();
                        axis->set_axis_id(i);
                        axis->set_position(latestFeedback.position[i]);
                        axis->set_cmd_position(latestFeedback.cmdPosition[i]);
                        axis->set_cmd_velocity(latestFeedback.cmdVelocity[i]);
                        axis->set_velocity(latestFeedback.velocity[i]);
                        axis->set_torque(latestFeedback.torque[i]);
                    }
                }

                status.set_heartbeat(bridge_->heartbeat());
                status.set_dropped_commands(bridge_->droppedCount());

                // 写入 taskSched 状态供 GUI 显示
                auto ts = bridge_->getTaskScheduling();
                switch (ts) {
                    case zrcs::TaskScheduling::START:       status.set_system_state("START"); break;
                    case zrcs::TaskScheduling::RUN:         status.set_system_state("RUN"); break;
                    case zrcs::TaskScheduling::STOP:        status.set_system_state("STOP"); break;
                    case zrcs::TaskScheduling::ERROR_STATE: status.set_system_state("ERROR"); break;
                    case zrcs::TaskScheduling::RESET:       status.set_system_state("RESET"); break;
                    case zrcs::TaskScheduling::SHUTDOWN:    status.set_system_state("SHUTDOWN"); break;
                }
                for (const auto& entry : rtLogs) {
                    auto* log = status.add_rt_logs();
                    log->set_timestamp_us(entry.timestamp_us);
                    log->set_level(entry.level);
                    log->set_file(entry.file);
                    log->set_line(entry.line);
                    log->set_message(entry.message);
                }

                std::string serialized;
                if (status.SerializeToString(&serialized)) {
                    try {
                        zmq::message_t msg(serialized.size());
                        memcpy(msg.data(), serialized.data(), serialized.size());
                        pub_socket_->send(msg, zmq::send_flags::dontwait);
                    } catch (const zmq::error_t& e) {
                        if (running_) {
                            spdlog::warn("[StatusPublisher] Send error: {}", e.what());
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(PUB_INTERVAL_MS));
        }
    }
};

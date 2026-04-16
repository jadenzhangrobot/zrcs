#pragma once

#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <fstream>
#include <chrono>

#include <spdlog/spdlog.h>

#include "message.pb.h"
#include "rt_bridge/RtBridge.h"
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

private:
    static constexpr const char* PUB_ENDPOINT = "tcp://*:5556";
    static constexpr int PUB_INTERVAL_MS = 10;

    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> pub_socket_;
    RtBridge* bridge_;
    std::thread pub_thread_;
    std::atomic<bool> running_;

    void run() {
        zrcs::AxisFeedbackData feedback{};

        // 轴数据 CSV 日志
        std::ofstream axisLog("axis_log.csv", std::ios::out | std::ios::trunc);
        if (axisLog.is_open()) {
            axisLog << "time_ms,x_pos,y_pos,z_pos,x_cmd,y_cmd,z_cmd,x_vel,y_vel,z_vel\n";
        }
        auto startTime = std::chrono::steady_clock::now();

        while (running_) {
            if (bridge_->readLatestAxisFeedback(feedback)) {
                zrcs_message::SystemStatus status;
                uint8_t count = bridge_->axisCount();
                if (count == 0) count = AXISMAXCOUNT;

                for (uint8_t i = 0; i < count; ++i) {
                    auto* axis = status.add_axes();
                    axis->set_axis_id(i);
                    axis->set_position(feedback.position[i]);
                    axis->set_cmd_position(feedback.cmdPosition[i]);
                    axis->set_velocity(feedback.velocity[i]);
                    axis->set_torque(feedback.torque[i]);
                }

                // 写入前 3 轴（x/y/z）数据到 CSV
                if (axisLog.is_open() && count >= 3) {
                    auto elapsed = std::chrono::steady_clock::now() - startTime;
                    double ms = std::chrono::duration<double, std::milli>(elapsed).count();
                    axisLog << ms << ","
                            << feedback.position[0] << ","
                            << feedback.position[1] << ","
                            << feedback.position[2] << ","
                            << feedback.cmdPosition[0] << ","
                            << feedback.cmdPosition[1] << ","
                            << feedback.cmdPosition[2] << ","
                            << feedback.velocity[0] << ","
                            << feedback.velocity[1] << ","
                            << feedback.velocity[2] << "\n";
                }

                status.set_heartbeat(bridge_->heartbeat());
                status.set_dropped_commands(bridge_->droppedCount());

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

/**
 * @file StatusPublisher.cpp
 * @brief 统一状态采集与发布线程：drain SPSC 队列 → protobuf → ZMQ PUB。
 */
#include "status/StatusPublisher.h"

#include <chrono>
#include <cstring>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "behavior_tree/core/BehaviorTreeService.h"
#include "message.pb.h"
#include "rtBridge/RtBridge.h"

namespace {

// ── protobuf 序列化 helper ───────────────────────────────────────────────

void appendFeedbackFrame(const SystemStatusSnapshot::AxisFrame& frame,
                         zrcs_message::SystemStatus& status)
{
    constexpr double kNanosecondsToSeconds = 1.0e-9;
    status.set_timestamp(static_cast<double>(frame.simulationTimeNs) *
                         kNanosecondsToSeconds);

    auto* fb = status.add_axis_feedback_frames();
    fb->set_sequence(frame.sequence);
    fb->set_simulation_time_ns(frame.simulationTimeNs);

    const size_t axisCount = frame.position.size();
    for (size_t i = 0; i < axisCount; ++i) {
        auto* axis = fb->add_axes();
        axis->set_axis_id(static_cast<uint8_t>(i));
        axis->set_position(frame.position[i]);
        axis->set_cmd_position(frame.cmdPosition[i]);
        axis->set_cmd_velocity(frame.cmdVelocity[i]);
        axis->set_velocity(frame.velocity[i]);
        axis->set_torque(frame.torque[i]);
        axis->set_servo_enabled(frame.servoEnabled[i] != 0);
    }
}

void appendMetadata(uint64_t heartbeat,
                    uint64_t droppedCommands,
                    const std::string& systemState,
                    const zrcs_nrt::BtStatus& bt,
                    const std::vector<zrcs::RtLogEntry>& logs,
                    zrcs_message::SystemStatus& status)
{
    status.set_heartbeat(heartbeat);
    status.set_dropped_commands(droppedCommands);
    status.set_system_state(systemState);

    for (const auto& entry : logs) {
        auto* log = status.add_rt_logs();
        log->set_timestamp_us(entry.timestamp_us);
        log->set_level(entry.level);
        log->set_file(entry.file);
        log->set_line(entry.line);
        log->set_message(entry.message);
    }

    auto* btMsg = status.mutable_bt_status();
    btMsg->set_tree_state(bt.treeState);
    btMsg->set_current_node(bt.currentNode);
    btMsg->set_message(bt.message);
}

bool sendStatus(zmq::socket_t& socket, const zrcs_message::SystemStatus& status)
{
    std::string serialized;
    if (!status.SerializeToString(&serialized)) {
        return false;
    }

    zmq::message_t msg(serialized.size());
    memcpy(msg.data(), serialized.data(), serialized.size());
    return socket.send(msg, zmq::send_flags::dontwait).has_value();
}

} // namespace

// ── StatusPublisher 实现 ─────────────────────────────────────────────────

StatusPublisher::StatusPublisher(RtBridge* bridge,
                                 BehaviorTreeService* behaviorTree)
    : context_(1)
    , collector_(bridge, behaviorTree)
{
}

StatusPublisher::~StatusPublisher()
{
    stop();
}

bool StatusPublisher::initialize()
{
    std::lock_guard<std::mutex> lock(stopMutex_);
    try {
        pub_socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::pub);
        pub_socket_->set(zmq::sockopt::linger, 0);
        pub_socket_->set(zmq::sockopt::sndhwm, 4096);
        pub_socket_->bind(PUB_ENDPOINT);
        stopped_ = false;
        spdlog::info("[StatusPublisher] Initialized on {}", PUB_ENDPOINT);
        return true;
    } catch (const zmq::error_t& e) {
        spdlog::error("[StatusPublisher] Init error: {}", e.what());
        return false;
    }
}

void StatusPublisher::start()
{
    std::lock_guard<std::mutex> lock(stopMutex_);
    if (running_.exchange(true)) {
        return;
    }
    stopped_ = false;
    pub_thread_ = std::thread(&StatusPublisher::run, this);
    spdlog::info("[StatusPublisher] Started (interval={}ms)", PUB_INTERVAL_MS);
}

void StatusPublisher::stop()
{
    std::lock_guard<std::mutex> lock(stopMutex_);
    if (stopped_) {
        return;
    }

    running_.store(false, std::memory_order_release);

    if (pub_thread_.joinable()) {
        if (pub_thread_.get_id() != std::this_thread::get_id()) {
            pub_thread_.join();
        } else {
            pub_thread_.detach();
        }
    }

    if (pub_socket_) {
        try { pub_socket_->close(); } catch (...) {}
        pub_socket_.reset();
    }

    try { context_.close(); } catch (...) {}

    stopped_ = true;
    spdlog::info("[StatusPublisher] Stopped");
}

void StatusPublisher::enqueueRtLog(const zrcs::RtLogEntry& entry)
{
    std::lock_guard<std::mutex> lock(logMutex_);
    if (pendingLogs_.size() >= kMaxPendingLogs) {
        pendingLogs_.pop_front();
    }
    pendingLogs_.push_back(entry);
}

void StatusPublisher::run()
{
    double lastTimestampSec = 0.0;

    while (running_.load(std::memory_order_acquire)) {
        if (!pub_socket_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(PUB_INTERVAL_MS));
            continue;
        }

        try {
            // 数据采集委托给 StatusCollector（采集与发送分离）
            auto snapshot = collector_.collect(/*drainAxisFeedback=*/true,
                                               /*drainRtLogs=*/false);

            // RT 日志仍由 enqueueRtLog() 推入内部队列，此处搬空
            std::vector<zrcs::RtLogEntry> logs;
            {
                std::lock_guard<std::mutex> lock(logMutex_);
                logs.reserve(pendingLogs_.size());
                while (!pendingLogs_.empty()) {
                    logs.push_back(pendingLogs_.front());
                    pendingLogs_.pop_front();
                }
            }

            const bool hasMeta = !logs.empty() ||
                                 !snapshot.btStatus.treeState.empty() ||
                                 !snapshot.btStatus.currentNode.empty() ||
                                 !snapshot.btStatus.message.empty();

            for (size_t i = 0; i < snapshot.axisFrames.size(); ++i) {
                zrcs_message::SystemStatus msg;
                appendFeedbackFrame(snapshot.axisFrames[i], msg);
                lastTimestampSec = msg.timestamp();

                if (i + 1 == snapshot.axisFrames.size()) {
                    appendMetadata(snapshot.heartbeat, snapshot.droppedCommands,
                                  snapshot.systemState, snapshot.btStatus, logs, msg);
                }

                if (!sendStatus(*pub_socket_, msg)) {
                    spdlog::warn("[StatusPublisher] Frame send dropped: sequence={}",
                                 snapshot.axisFrames[i].sequence);
                }
            }

            if (snapshot.axisFrames.empty() && hasMeta) {
                zrcs_message::SystemStatus msg;
                msg.set_timestamp(lastTimestampSec);
                appendMetadata(snapshot.heartbeat, snapshot.droppedCommands,
                              snapshot.systemState, snapshot.btStatus, logs, msg);
                if (!sendStatus(*pub_socket_, msg)) {
                    spdlog::warn("[StatusPublisher] Metadata send dropped");
                }
            }
        } catch (const zmq::error_t& e) {
            if (running_.load(std::memory_order_acquire)) {
                spdlog::warn("[StatusPublisher] Send error: {}", e.what());
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(PUB_INTERVAL_MS));
    }
}

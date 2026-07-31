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

void appendAxis(const zrcs::AxisFeedbackData& source,
                size_t axisIndex,
                zrcs_message::AxisStatus* target)
{
    target->set_axis_id(static_cast<uint8_t>(axisIndex));
    target->set_position(source.position[axisIndex]);
    target->set_cmd_position(source.cmdPosition[axisIndex]);
    target->set_cmd_velocity(source.cmdVelocity[axisIndex]);
    target->set_velocity(source.velocity[axisIndex]);
    target->set_torque(source.torque[axisIndex]);
}

void appendFeedbackFrame(const zrcs::AxisFeedbackData& frame,
                         zrcs_message::SystemStatus& status)
{
    constexpr double kNanosecondsToSeconds = 1.0e-9;
    status.set_timestamp(static_cast<double>(frame.simulationTimeNs) *
                         kNanosecondsToSeconds);

    auto* fb = status.add_axis_feedback_frames();
    fb->set_sequence(frame.sequence);
    fb->set_simulation_time_ns(frame.simulationTimeNs);

    const size_t axisCount = std::min(
        static_cast<size_t>(frame.axisCount), zrcs::kAxisMax);
    for (size_t i = 0; i < axisCount; ++i) {
        appendAxis(frame, i, fb->add_axes());
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
    , bridge_(bridge)
    , behaviorTree_(behaviorTree)
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

std::string StatusPublisher::taskStateToString(zrcs::TaskScheduling ts)
{
    using TS = zrcs::TaskScheduling;
    switch (ts) {
    case TS::IDLE:        return "IDLE";
    case TS::RUN:          return "RUN";
    case TS::STOP:         return "STOP";
    case TS::ERROR_STATE:  return "ERROR";
    case TS::RESET:        return "RESET";
    case TS::SHUTDOWN:     return "SHUTDOWN";
    default:               return "IDLE";
    }
}

void StatusPublisher::run()
{
    double lastTimestampSec = 0.0;

    while (running_.load(std::memory_order_acquire)) {
        if (!bridge_ || !pub_socket_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(PUB_INTERVAL_MS));
            continue;
        }

        try {
            // 1. 从 SPSC 队列 drain 所有轴反馈帧
            zrcs::AxisFeedbackData rawFrame{};
            std::vector<zrcs::AxisFeedbackData> frames;
            frames.reserve(64);
            while (bridge_->readLatestAxisFeedback(rawFrame)) {
                frames.push_back(rawFrame);
            }

            // 2. 周期读取系统元数据（原子操作，直接读 shared memory）
            const auto heartbeat = bridge_->heartbeat();
            const auto dropped = bridge_->droppedCount();
            const auto sysState = taskStateToString(bridge_->getTaskScheduling());

            // 3. 读取 BT 状态（BehaviorTreeService::status() 内部有锁）
            zrcs_nrt::BtStatus bt;
            if (behaviorTree_) {
                bt = behaviorTree_->status();
            }

            // 4. 搬空 RT 日志队列
            std::vector<zrcs::RtLogEntry> logs;
            {
                std::lock_guard<std::mutex> lock(logMutex_);
                logs.reserve(pendingLogs_.size());
                while (!pendingLogs_.empty()) {
                    logs.push_back(pendingLogs_.front());
                    pendingLogs_.pop_front();
                }
            }

            // 5. 反馈帧逐条发送，元数据附在最后一帧上
            const bool hasMeta = !logs.empty() ||
                                 !bt.treeState.empty() ||
                                 !bt.currentNode.empty() ||
                                 !bt.message.empty();

            for (size_t i = 0; i < frames.size(); ++i) {
                zrcs_message::SystemStatus msg;
                appendFeedbackFrame(frames[i], msg);
                lastTimestampSec = msg.timestamp();

                if (i + 1 == frames.size()) {
                    appendMetadata(heartbeat, dropped, sysState, bt, logs, msg);
                }

                if (!sendStatus(*pub_socket_, msg)) {
                    spdlog::warn("[StatusPublisher] Frame send dropped: sequence={}",
                                 frames[i].sequence);
                }
            }

            // 6. 无反馈帧时，若有元数据变更仍需单独发送
            if (frames.empty() && hasMeta) {
                zrcs_message::SystemStatus msg;
                msg.set_timestamp(lastTimestampSec);
                appendMetadata(heartbeat, dropped, sysState, bt, logs, msg);
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

#include "nrtServer/zmq/StatusPublisher.h"

#include <chrono>
#include <cstring>
#include <string>

#include <spdlog/spdlog.h>

#include "message.pb.h"
#include "status/StatusStore.h"

namespace {

void appendAxis(const zrcs_nrt::AxisStatusSnapshot& source,
                zrcs_message::AxisStatus* target)
{
    target->set_axis_id(source.axisId);
    target->set_position(source.position);
    target->set_cmd_position(source.cmdPosition);
    target->set_cmd_velocity(source.cmdVelocity);
    target->set_velocity(source.velocity);
    target->set_torque(source.torque);
}

void appendFrame(const zrcs_nrt::AxisFeedbackFrameSnapshot& frame,
                 zrcs_message::SystemStatus& status)
{
    constexpr double kNanosecondsToSeconds = 1.0e-9;
    status.set_timestamp(static_cast<double>(frame.simulationTimeNs) *
                         kNanosecondsToSeconds);

    auto* feedback = status.add_axis_feedback_frames();
    feedback->set_sequence(frame.sequence);
    feedback->set_simulation_time_ns(frame.simulationTimeNs);

    for (const auto& axis : frame.axes) {
        appendAxis(axis, status.add_axes());
        appendAxis(axis, feedback->add_axes());
    }
}

void appendMetadata(const zrcs_nrt::SystemStatusSnapshot& snap,
                    zrcs_message::SystemStatus& status)
{
    status.set_heartbeat(snap.heartbeat);
    status.set_dropped_commands(snap.droppedCommands);
    status.set_system_state(snap.systemState);

    for (const auto& entry : snap.rtLogs) {
        auto* log = status.add_rt_logs();
        log->set_timestamp_us(entry.timestamp_us);
        log->set_level(entry.level);
        log->set_file(entry.file);
        log->set_line(entry.line);
        log->set_message(entry.message);
    }

    auto* bt = status.mutable_bt_status();
    bt->set_tree_state(snap.bt.treeState);
    bt->set_current_node(snap.bt.currentNode);
    bt->set_message(snap.bt.message);
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

StatusPublisher::StatusPublisher(StatusStore* store)
    : context_(1)
    , store_(store)
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
        try {
            pub_socket_->close();
        } catch (...) {
        }
        pub_socket_.reset();
    }

    try {
        context_.close();
    } catch (...) {
    }

    stopped_ = true;
    spdlog::info("[StatusPublisher] Stopped");
}

void StatusPublisher::run()
{
    double lastFeedbackTimestampSec = 0.0;

    while (running_.load(std::memory_order_acquire)) {
        if (store_ && pub_socket_) {
            const auto snap = store_->snapshot();
            // 反馈帧逐条发送；元数据附在最后一帧上，避免日志被重复发送。
            const bool hasBt = !snap.bt.treeState.empty() ||
                               !snap.bt.currentNode.empty() ||
                               !snap.bt.message.empty();
            try {
                for (size_t i = 0; i < snap.axisFeedbackFrames.size(); ++i) {
                    zrcs_message::SystemStatus status;
                    appendFrame(snap.axisFeedbackFrames[i], status);
                    lastFeedbackTimestampSec = status.timestamp();
                    if (i + 1 == snap.axisFeedbackFrames.size()) {
                        appendMetadata(snap, status);
                    }
                    if (!sendStatus(*pub_socket_, status)) {
                        spdlog::warn("[StatusPublisher] Feedback frame send dropped: sequence={}",
                                     snap.axisFeedbackFrames[i].sequence);
                    }
                }

                // 无新反馈时仍允许系统状态、BT 和日志更新，但不重复发送旧 axes。
                if (snap.axisFeedbackFrames.empty() &&
                    (!snap.rtLogs.empty() || hasBt || !snap.systemState.empty())) {
                    zrcs_message::SystemStatus status;
                    status.set_timestamp(lastFeedbackTimestampSec);
                    appendMetadata(snap, status);
                    if (!sendStatus(*pub_socket_, status)) {
                        spdlog::warn("[StatusPublisher] Metadata send dropped");
                    }
                }
            } catch (const zmq::error_t& e) {
                if (running_.load(std::memory_order_acquire)) {
                    spdlog::warn("[StatusPublisher] Send error: {}", e.what());
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(PUB_INTERVAL_MS));
    }
}

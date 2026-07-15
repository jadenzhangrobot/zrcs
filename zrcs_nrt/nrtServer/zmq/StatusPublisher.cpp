#include "nrtServer/zmq/StatusPublisher.h"

#include <chrono>
#include <cstring>
#include <string>

#include <spdlog/spdlog.h>

#include "message.pb.h"
#include "status/StatusStore.h"

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
        pub_socket_->set(zmq::sockopt::sndhwm, 100);
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
    while (running_.load(std::memory_order_acquire)) {
        if (store_ && pub_socket_) {
            const auto snap = store_->snapshot();
            // 有轴反馈、日志，或任意系统/BT 状态时都发布，保证 GUI 能拿到行为树状态
            const bool hasBt = !snap.bt.treeState.empty() ||
                               !snap.bt.currentNode.empty() ||
                               !snap.bt.message.empty();
            if (snap.hasAxisFeedback || !snap.rtLogs.empty() || hasBt ||
                !snap.systemState.empty()) {
                zrcs_message::SystemStatus status;
                for (const auto& a : snap.axes) {
                    auto* axis = status.add_axes();
                    axis->set_axis_id(a.axisId);
                    axis->set_position(a.position);
                    axis->set_cmd_position(a.cmdPosition);
                    axis->set_cmd_velocity(a.cmdVelocity);
                    axis->set_velocity(a.velocity);
                    axis->set_torque(a.torque);
                }
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

                std::string serialized;
                if (status.SerializeToString(&serialized)) {
                    try {
                        zmq::message_t msg(serialized.size());
                        memcpy(msg.data(), serialized.data(), serialized.size());
                        pub_socket_->send(msg, zmq::send_flags::dontwait);
                    } catch (const zmq::error_t& e) {
                        if (running_.load(std::memory_order_acquire)) {
                            spdlog::warn("[StatusPublisher] Send error: {}", e.what());
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(PUB_INTERVAL_MS));
    }
}

#include "statusPublisher/StatusPublisher.h"

#include <chrono>
#include <cstring>
#include <string>

#include <spdlog/spdlog.h>

#include "command/RtBridge.h"
#include "message.pb.h"

StatusPublisher::StatusPublisher(RtBridge* bridge)
    : context_(1)
    , bridge_(bridge)
    , running_(false)
{
}

StatusPublisher::~StatusPublisher()
{
    stop();
}

bool StatusPublisher::initialize()
{
    try
    {
        pub_socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::pub);
        pub_socket_->set(zmq::sockopt::linger, 0);
        pub_socket_->set(zmq::sockopt::sndhwm, 100);
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

void StatusPublisher::start()
{
    if (running_.exchange(true)) return;
    pub_thread_ = std::thread(&StatusPublisher::run, this);
    spdlog::info("[StatusPublisher] Started (interval={}ms)", PUB_INTERVAL_MS);
}

void StatusPublisher::stop()
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
    if (wasRunning || hasThread)
    {
        spdlog::info("[StatusPublisher] Stopped");
    }
}

void StatusPublisher::enqueueRtLog(const zrcs::RtLogEntry& entry)
{
    std::lock_guard<std::mutex> lk(log_mutex_);
    if (pending_logs_.size() >= kMaxPendingRtLogs)
    {
        pending_logs_.pop_front();
    }
    pending_logs_.push_back(entry);
}

std::vector<zrcs::RtLogEntry> StatusPublisher::drainPendingRtLogs()
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

void StatusPublisher::run()
{
    zrcs::AxisFeedbackData latestFeedback{};
    bool hasFeedback = false;

    while (running_)
    {
        zrcs::AxisFeedbackData feedback{};
        while (bridge_->readLatestAxisFeedback(feedback))
        {
            latestFeedback = feedback;
            hasFeedback = true;
        }

        auto rtLogs = drainPendingRtLogs();

        if (hasFeedback || !rtLogs.empty())
        {
            zrcs_message::SystemStatus status;
            uint8_t count = bridge_->axisCount();

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

            status.set_heartbeat(bridge_->heartbeat());
            status.set_dropped_commands(bridge_->droppedCount());

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

            for (const auto& entry : rtLogs)
            {
                auto* log = status.add_rt_logs();
                log->set_timestamp_us(entry.timestamp_us);
                log->set_level(entry.level);
                log->set_file(entry.file);
                log->set_line(entry.line);
                log->set_message(entry.message);
            }

            std::string serialized;
            if (status.SerializeToString(&serialized))
            {
                try
                {
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

        std::this_thread::sleep_for(std::chrono::milliseconds(PUB_INTERVAL_MS));
    }
}

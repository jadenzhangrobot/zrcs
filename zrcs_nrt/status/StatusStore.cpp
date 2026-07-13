#include "status/StatusStore.h"

void StatusStore::updateAxisFeedback(const zrcs::AxisFeedbackData& feedback, uint8_t axisCount)
{
    std::lock_guard<std::mutex> lock(mutex_);
    latestFeedback_ = feedback;
    hasFeedback_ = true;
    axisCount_ = axisCount;
}

void StatusStore::updateSystemMeta(uint64_t heartbeat,
                                   uint64_t droppedCommands,
                                   const std::string& systemState)
{
    std::lock_guard<std::mutex> lock(mutex_);
    heartbeat_ = heartbeat;
    droppedCommands_ = droppedCommands;
    systemState_ = systemState;
}

void StatusStore::setBtStatus(const zrcs_nrt::BtStatusSnapshot& bt)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bt_ = bt;
}

void StatusStore::enqueueRtLog(const zrcs::RtLogEntry& entry)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (pendingLogs_.size() >= kMaxPendingRtLogs) {
        pendingLogs_.pop_front();
    }
    pendingLogs_.push_back(entry);
}

zrcs_nrt::SystemStatusSnapshot StatusStore::snapshot()
{
    std::lock_guard<std::mutex> lock(mutex_);
    zrcs_nrt::SystemStatusSnapshot out;
    out.hasAxisFeedback = hasFeedback_;
    out.heartbeat = heartbeat_;
    out.droppedCommands = droppedCommands_;
    out.systemState = systemState_;
    out.bt = bt_;

    if (hasFeedback_) {
        out.axes.reserve(axisCount_);
        for (uint8_t i = 0; i < axisCount_; ++i) {
            zrcs_nrt::AxisStatusSnapshot axis;
            axis.axisId = i;
            axis.position = latestFeedback_.position[i];
            axis.cmdPosition = latestFeedback_.cmdPosition[i];
            axis.cmdVelocity = latestFeedback_.cmdVelocity[i];
            axis.velocity = latestFeedback_.velocity[i];
            axis.torque = latestFeedback_.torque[i];
            out.axes.push_back(axis);
        }
    }

    out.rtLogs.reserve(pendingLogs_.size());
    while (!pendingLogs_.empty()) {
        out.rtLogs.push_back(pendingLogs_.front());
        pendingLogs_.pop_front();
    }
    return out;
}

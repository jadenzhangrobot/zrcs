#include "status/StatusStore.h"

#include <algorithm>
#include <utility>

namespace {

std::vector<zrcs_nrt::AxisStatusSnapshot> makeAxesSnapshot(
    const zrcs::AxisFeedbackData& feedback)
{
    const size_t count = std::min(static_cast<size_t>(feedback.axisCount), zrcs::kAxisMax);
    std::vector<zrcs_nrt::AxisStatusSnapshot> axes;
    axes.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        zrcs_nrt::AxisStatusSnapshot axis;
        axis.axisId = static_cast<uint8_t>(i);
        axis.position = feedback.position[i];
        axis.cmdPosition = feedback.cmdPosition[i];
        axis.cmdVelocity = feedback.cmdVelocity[i];
        axis.velocity = feedback.velocity[i];
        axis.torque = feedback.torque[i];
        axes.push_back(axis);
    }
    return axes;
}

} // namespace

void StatusStore::updateAxisFeedbackBatch(
    const std::vector<zrcs::AxisFeedbackData>& feedbackBatch)
{
    if (feedbackBatch.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    latestFeedback_ = feedbackBatch.back();
    hasFeedback_ = true;

    for (const auto& feedback : feedbackBatch) {
        if (pendingAxisFeedback_.size() >= kMaxPendingAxisFeedback) {
            pendingAxisFeedback_.pop_front();
        }
        pendingAxisFeedback_.push_back(feedback);
    }
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
        out.axes = makeAxesSnapshot(latestFeedback_);
    }

    out.axisFeedbackFrames.reserve(pendingAxisFeedback_.size());
    while (!pendingAxisFeedback_.empty()) {
        const auto& feedback = pendingAxisFeedback_.front();
        zrcs_nrt::AxisFeedbackFrameSnapshot frame;
        frame.sequence = feedback.sequence;
        frame.simulationTimeNs = feedback.simulationTimeNs;
        frame.axes = makeAxesSnapshot(feedback);
        out.axisFeedbackFrames.push_back(std::move(frame));
        pendingAxisFeedback_.pop_front();
    }

    out.rtLogs.reserve(pendingLogs_.size());
    while (!pendingLogs_.empty()) {
        out.rtLogs.push_back(pendingLogs_.front());
        pendingLogs_.pop_front();
    }
    return out;
}

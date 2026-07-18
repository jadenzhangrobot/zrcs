#pragma once
/**
 * @file StatusStore.h
 * @brief 线程安全的系统/行为树状态缓存。
 */

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "shared_memory/ShmLayout.h"
#include "status/StatusTypes.h"

class StatusStore {
public:
    static constexpr size_t kMaxPendingRtLogs = 256;
    static constexpr size_t kMaxPendingAxisFeedback = zrcs::kLogQueueCap;

    void updateAxisFeedbackBatch(const std::vector<zrcs::AxisFeedbackData>& feedbackBatch);
    void updateSystemMeta(uint64_t heartbeat,
                          uint64_t droppedCommands,
                          const std::string& systemState);
    void setBtStatus(const zrcs_nrt::BtStatusSnapshot& bt);
    void enqueueRtLog(const zrcs::RtLogEntry& entry);

    /// 取出完整快照；反馈批次和日志取出后从待发布队列清空。
    zrcs_nrt::SystemStatusSnapshot snapshot();

private:
    mutable std::mutex mutex_;
    zrcs::AxisFeedbackData latestFeedback_{};
    bool hasFeedback_ = false;
    uint64_t heartbeat_ = 0;
    uint64_t droppedCommands_ = 0;
    std::string systemState_ = "IDLE";
    zrcs_nrt::BtStatusSnapshot bt_{};
    std::deque<zrcs::AxisFeedbackData> pendingAxisFeedback_;
    std::deque<zrcs::RtLogEntry> pendingLogs_;
};

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

    void updateAxisFeedback(const zrcs::AxisFeedbackData& feedback, uint8_t axisCount);
    void updateSystemMeta(uint64_t heartbeat,
                          uint64_t droppedCommands,
                          const std::string& systemState);
    void setBtStatus(const zrcs_nrt::BtStatusSnapshot& bt);
    void enqueueRtLog(const zrcs::RtLogEntry& entry);

    /// 取出完整快照；rtLogs 为自上次 snapshot 以来累计的日志（取出后清空待发队列）。
    zrcs_nrt::SystemStatusSnapshot snapshot();

private:
    mutable std::mutex mutex_;
    zrcs::AxisFeedbackData latestFeedback_{};
    bool hasFeedback_ = false;
    uint8_t axisCount_ = 0;
    uint64_t heartbeat_ = 0;
    uint64_t droppedCommands_ = 0;
    std::string systemState_ = "IDLE";
    zrcs_nrt::BtStatusSnapshot bt_{};
    std::deque<zrcs::RtLogEntry> pendingLogs_;
};

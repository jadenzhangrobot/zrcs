#pragma once
/**
 * @file StatusStore.h
 * @brief 线程安全的系统/行为树状态缓存。
 *
 * 定位：生产者与消费者之间的一个带锁缓冲区。
 *   写入方（多个，各自独立线程）：
 *     - StatusCollector      -> updateAxisFeedbackBatch / updateSystemMeta
 *     - BehaviorTreeService  -> setBtStatus
 *     - RtLogCollector 等    -> enqueueRtLog
 *   读取方（单个）：
 *     - StatusPublisher      -> snapshot()
 *
 * 两类字段的语义不同，用的时候要分清：
 *   - 「当前值」字段（heartbeat_/systemState_/bt_）：后写覆盖前写，snapshot()
 *     只是拷一份，可以被反复读到。
 *   - 「待发布队列」字段（pendingAxisFeedback_/pendingLogs_）：snapshot() 会
 *     把它们搬空，同一份数据只会被消费一次。这是为了保证波形不重画、日志不重复。
 *
 * 这里用普通 std::mutex 而不是无锁结构：本类运行在 NRT 侧，没有实时性约束，
 * 临界区内只做定长结构体的拷贝和 deque 的收尾操作，持锁时间足够短。
 * RT 与 NRT 之间的跨进程传递才需要无锁，那部分在 ShmSPSCQueue 里。
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
    /// RT 日志待发布队列上限。超出后丢弃最旧的一条（见 enqueueRtLog）。
    static constexpr size_t kMaxPendingRtLogs = 256;

    /// 轴反馈待发布队列上限。取和共享内存日志队列同样的容量（1024），
    /// 使 NRT 侧缓冲深度与 RT 侧产生能力量级一致；超出后丢弃最旧的一帧。
    static constexpr size_t kMaxPendingAxisFeedback = zrcs::kLogQueueCap;

    /**
     * @brief 批量写入轴反馈帧。
     * @param feedbackBatch 一次采集从 RT 队列里取空得到的连续多帧，按时间先后排列
     *
     * 传批次而不是单帧，是因为 RT 侧 1ms 产生一帧、采集线程 10ms 才醒一次，
     * 一次唤醒通常能取到十帧左右，逐帧调用会反复加解锁。
     */
    void updateAxisFeedbackBatch(const std::vector<zrcs::AxisFeedbackData>& feedbackBatch);

    /**
     * @brief 更新系统级元数据（当前值语义，覆盖写）。
     * @param heartbeat       RT 心跳计数
     * @param droppedCommands 命令丢弃累计数
     * @param systemState     任务调度状态字符串，见 StatusCollector::taskStateToString
     */
    void updateSystemMeta(uint64_t heartbeat,
                          uint64_t droppedCommands,
                          const std::string& systemState);

    /// 更新行为树状态（当前值语义，覆盖写）。
    void setBtStatus(const zrcs_nrt::BtStatus& bt);

    /// 追加一条 RT 日志到待发布队列；队列满时丢弃最旧一条。
    void enqueueRtLog(const zrcs::RtLogEntry& entry);

    /// 取出完整快照；反馈批次和日志取出后从待发布队列清空。
    zrcs_nrt::SystemStatus snapshot();

private:
    /// 保护下面所有成员。mutable 是为了让将来可能出现的 const 读方法也能加锁。
    mutable std::mutex mutex_;

    uint64_t heartbeat_ = 0;
    uint64_t droppedCommands_ = 0;
    /// 初值取 IDLE，避免 RT 尚未连上时 GUI 显示空状态。
    std::string systemState_ = "IDLE";
    zrcs_nrt::BtStatus bt_{};

    /// 待发布的轴反馈帧。用 deque 是因为要在队首丢弃、队尾追加。
    std::deque<zrcs::AxisFeedbackData> pendingAxisFeedback_;
    /// 待发布的 RT 日志，同上。
    std::deque<zrcs::RtLogEntry> pendingLogs_;
};

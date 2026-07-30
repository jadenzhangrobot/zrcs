/**
 * @file StatusStore.cpp
 * @brief StatusStore 实现 —— 状态缓存的加锁读写。
 */
#include "status/StatusStore.h"

#include <algorithm>
#include <utility>

namespace {

/**
 * @brief 把共享内存里的定长数组布局转成变长的帧内轴状态。
 *
 * RT 侧的 AxisFeedbackData 为了能放进共享内存、避免动态分配，用的是
 * double position[kAxisMax] 这样的定长数组（kAxisMax = 64），实际轴数由
 * axisCount 指出。这里只截取前 axisCount 个有效元素。
 *
 * min(axisCount, kAxisMax) 是一道防线：axisCount 来自另一个进程写入的共享内存，
 * 万一 RT 侧异常写入越界值，这里钳一下可以避免读到数组外的内存。
 *
 * 注意 axisId 直接取下标 i，隐含假设 RT 侧是按轴号顺序密排填充的。
 * axis.xml 里轴号有空洞时（比如只配了 0 和 2），这个假设不成立。
 */
std::vector<zrcs_nrt::AxisStatus> makeAxesSnapshot(
    const zrcs::AxisFeedbackData& feedback)
{
    const size_t count = std::min(static_cast<size_t>(feedback.axisCount), zrcs::kAxisMax);
    std::vector<zrcs_nrt::AxisStatus> axes;
    axes.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        zrcs_nrt::AxisStatus axis;
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

    // 逐帧入队，队列满时丢最旧的。
    // 丢旧留新而不是丢新：波形显示更关心近期数据，积压说明发布端跟不上，
    // 此时保留最新帧能让界面尽快追上真实状态。
    for (const auto& feedback : feedbackBatch) {
        if (pendingAxisFeedback_.size() >= kMaxPendingAxisFeedback) {
            pendingAxisFeedback_.pop_front();
        }
        pendingAxisFeedback_.push_back(feedback);
    }
}

// 以下三个 update/set 都是「当前值」语义：直接覆盖，不排队。
// GUI 只需要知道最新状态，中间过程不必逐个送达。

void StatusStore::updateSystemMeta(uint64_t heartbeat,
                                   uint64_t droppedCommands,
                                   const std::string& systemState)
{
    std::lock_guard<std::mutex> lock(mutex_);
    heartbeat_ = heartbeat;
    droppedCommands_ = droppedCommands;
    systemState_ = systemState;
}

void StatusStore::setBtStatus(const zrcs_nrt::BtStatus& bt)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bt_ = bt;
}

void StatusStore::enqueueRtLog(const zrcs::RtLogEntry& entry)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // 日志是「队列」语义，每条都要送到，所以不能覆盖。
    // 但也不能无限增长，队列满时同样丢最旧的（RT 侧 logPush 队列满时的做法一致）。
    if (pendingLogs_.size() >= kMaxPendingRtLogs) {
        pendingLogs_.pop_front();
    }
    pendingLogs_.push_back(entry);
}

zrcs_nrt::SystemStatus StatusStore::snapshot()
{
    std::lock_guard<std::mutex> lock(mutex_);
    zrcs_nrt::SystemStatus out;

    // —— 当前值部分：拷贝，不清空，下次还能读到 ——
    out.heartbeat = heartbeat_;
    out.droppedCommands = droppedCommands_;
    out.systemState = systemState_;
    out.bt = bt_;

    // —— 队列部分：搬空，交接所有权 ——
    // 出队顺序即时间顺序（由旧到新），发布端依赖这个顺序保证波形横轴单调。
    out.axisFeedbackFrames.reserve(pendingAxisFeedback_.size());
    while (!pendingAxisFeedback_.empty()) {
        const auto& feedback = pendingAxisFeedback_.front();
        zrcs_nrt::AxisFeedbackFrame frame;
        // sequence / simulationTimeNs 从共享内存原样带出，供丢帧检测和时间对齐用。
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

    // 返回值走移动构造，不会在持锁期间发生大块拷贝。
    return out;
}

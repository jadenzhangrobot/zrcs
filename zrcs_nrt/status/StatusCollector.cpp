/**
 * @file StatusCollector.cpp
 * @brief StatusCollector 实现 —— 周期性拉取 RT 状态。
 */
#include "status/StatusCollector.h"

#include "behavior_tree/core/BehaviorTreeService.h"
#include "rtBridge/RtBridge.h"
#include "shared_memory/ShmLayout.h"
#include "status/StatusStore.h"

#include <vector>

StatusCollector::StatusCollector(RtBridge* bridge,
                                 StatusStore* store,
                                 BehaviorTreeService* behaviorTree,
                                 std::chrono::milliseconds interval)
    : bridge_(bridge)
    , store_(store)
    , behaviorTree_(behaviorTree)
    , interval_(interval)
{
}

StatusCollector::~StatusCollector()
{
    stop();
}

void StatusCollector::start()
{
    // exchange 返回旧值：已经是 true 说明线程已在跑，直接返回，
    // 避免重复 start 覆盖 thread_ 造成线程泄漏。
    if (running_.exchange(true)) {
        return;
    }
    thread_ = std::thread(&StatusCollector::run, this);
}

void StatusCollector::stop()
{
    // 只置标志，不强杀。线程最多再跑完当前这一轮采集就退出，
    // 最坏等待时间为一个 interval_（睡眠中无法提前唤醒）。
    running_.store(false, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
}

/// default 分支兜底到 IDLE：枚举值来自共享内存，可能是本进程不认识的新值，
/// 显示成 IDLE 比显示空串或崩溃更安全。
std::string StatusCollector::taskStateToString(zrcs::TaskScheduling ts)
{
    using TS = zrcs::TaskScheduling;
    switch (ts) {
    case TS::IDLE:
        return "IDLE";
    case TS::RUN:
        return "RUN";
    case TS::STOP:
        return "STOP";
    case TS::ERROR_STATE:
        return "ERROR";
    case TS::RESET:
        return "RESET";
    case TS::SHUTDOWN:
        return "SHUTDOWN";
    default:
        return "IDLE";
    }
}

void StatusCollector::run()
{
    while (running_.load(std::memory_order_acquire)) {
        if (bridge_ && store_) {
            zrcs::AxisFeedbackData feedback{};
            std::vector<zrcs::AxisFeedbackData> feedbackBatch;
            // 预留 64：10ms 周期、RT 1ms 一帧，正常约 10 帧，留足余量避免扩容。
            feedbackBatch.reserve(64);

            // 把 SPSC 队列一次抽空。
            // 注意 readLatestAxisFeedback 这个名字有误导：它实际是 pop()，
            // 返回的是队首（最旧）那一帧，不是"最新"帧。所以这个循环拿到的是
            // 上次采集以来的完整帧序列，按时间由旧到新排列。
            // 抽空而不是只取一帧，是为了不让队列积压到覆盖丢帧。
            while (bridge_->readLatestAxisFeedback(feedback)) {
                feedbackBatch.push_back(feedback);
            }
            if (!feedbackBatch.empty()) {
                store_->updateAxisFeedbackBatch(feedbackBatch);
            }

            // 元数据每轮都更新，即使没有新反馈帧。
            // 这样 RT 停止产生反馈时（比如 STOP 状态下），GUI 仍能看到心跳和状态变化。
            store_->updateSystemMeta(bridge_->heartbeat(),
                                     bridge_->droppedCount(),
                                     taskStateToString(bridge_->getTaskScheduling()));
        }

        // 周期同步行为树状态，保证 GUI 能在运行中看到 currentNode 变化
        // （BT 在自己的线程里 20ms 一 tick，不会主动推状态，只能这里拉）
        if (behaviorTree_) {
            behaviorTree_->syncStatusToStore();
        }

        // 固定间隔睡眠，不做补偿。采集有抖动不影响正确性：帧序列本身带
        // sequence 和时间戳，时序信息不依赖采集时刻。
        std::this_thread::sleep_for(interval_);
    }
}

#include "status/StatusCollector.h"

#include "behavior_tree/core/BehaviorTreeService.h"
#include "rtBridge/RtBridge.h"
#include "shared_memory/ShmLayout.h"
#include "status/StatusStore.h"

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
    if (running_.exchange(true)) {
        return;
    }
    thread_ = std::thread(&StatusCollector::run, this);
}

void StatusCollector::stop()
{
    running_.store(false, std::memory_order_release);
    if (thread_.joinable()) {
        thread_.join();
    }
}

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
            bool gotFeedback = false;
            while (bridge_->readLatestAxisFeedback(feedback)) {
                gotFeedback = true;
            }
            if (gotFeedback) {
                store_->updateAxisFeedback(feedback, bridge_->axisCount());
            }

            store_->updateSystemMeta(bridge_->heartbeat(),
                                     bridge_->droppedCount(),
                                     taskStateToString(bridge_->getTaskScheduling()));
        }

        // 周期同步行为树状态，保证 GUI 能在运行中看到 currentNode 变化
        if (behaviorTree_) {
            behaviorTree_->syncStatusToStore();
        }

        std::this_thread::sleep_for(interval_);
    }
}

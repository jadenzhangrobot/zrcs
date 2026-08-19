#include "status/StatusCollector.h"

#include <algorithm>

#include "behavior_tree/core/BehaviorTreeService.h"
#include "rtBridge/RtBridge.h"

// =============================================================================
// 构造
// =============================================================================

StatusCollector::StatusCollector(RtBridge* bridge,
                                 BehaviorTreeService* behaviorTree)
    : bridge_(bridge)
    , behaviorTree_(behaviorTree)
{
}

// =============================================================================
// 完整采集
// =============================================================================

SystemStatusSnapshot StatusCollector::collect(bool drainAxisFeedback,
                                               bool drainRtLogs)
{
    SystemStatusSnapshot snapshot;

    // 1) 轴反馈帧
    if (drainAxisFeedback) {
        snapshot.axisFrames = drainAxisFrames(bridge_);
    }

    // 2) 元数据
    readMetadata(bridge_, snapshot.heartbeat, snapshot.droppedCommands,
                 snapshot.systemState);

    // 3) 行为树状态
    if (behaviorTree_) {
        snapshot.btStatus = behaviorTree_->status();
    }

    return snapshot;
}

// =============================================================================
// 轴反馈帧 drain
// =============================================================================

std::vector<SystemStatusSnapshot::AxisFrame>
StatusCollector::drainAxisFrames(RtBridge* bridge)
{
    std::vector<SystemStatusSnapshot::AxisFrame> frames;
    if (!bridge) return frames;

    zrcs::AxisFeedbackData raw{};
    while (bridge->readLatestAxisFeedback(raw)) {
        SystemStatusSnapshot::AxisFrame frame;
        frame.sequence          = raw.sequence;
        frame.simulationTimeNs  = raw.simulationTimeNs;
        frame.axisCount         = raw.axisCount;

        const size_t n = std::min(static_cast<size_t>(raw.axisCount),
                                  zrcs::kAxisMax);
        frame.position.resize(n);
        frame.cmdPosition.resize(n);
        frame.cmdVelocity.resize(n);
        frame.velocity.resize(n);
        frame.torque.resize(n);
        frame.servoEnabled.resize(n);

        for (size_t i = 0; i < n; ++i) {
            frame.position[i]    = raw.position[i];
            frame.cmdPosition[i] = raw.cmdPosition[i];
            frame.cmdVelocity[i] = raw.cmdVelocity[i];
            frame.velocity[i]    = raw.velocity[i];
            frame.torque[i]      = raw.torque[i];
            frame.servoEnabled[i]= raw.servoEnabled[i];
        }

        frames.push_back(std::move(frame));
    }
    return frames;
}

// =============================================================================
// 元数据读取
// =============================================================================

void StatusCollector::readMetadata(RtBridge* bridge,
                                   uint64_t& heartbeat,
                                   uint64_t& dropped,
                                   std::string& systemState)
{
    if (!bridge) {
        heartbeat = 0;
        dropped = 0;
        return;
    }

    heartbeat = bridge->heartbeat();
    dropped   = bridge->droppedCount();

    // 任务调度状态 → 字符串
    switch (bridge->getTaskScheduling()) {
    case zrcs::TaskScheduling::IDLE:        systemState = "IDLE";        break;
    case zrcs::TaskScheduling::RUN:          systemState = "RUN";          break;
    case zrcs::TaskScheduling::STOP:         systemState = "STOP";         break;
    case zrcs::TaskScheduling::ERROR_STATE:  systemState = "ERROR";        break;
    case zrcs::TaskScheduling::RESET:        systemState = "RESET";        break;
    case zrcs::TaskScheduling::SHUTDOWN:     systemState = "SHUTDOWN";     break;
    default:                                 systemState = "IDLE";         break;
    }
}

#pragma once
/**
 * @file StatusTypes.h
 * @brief NRT 状态快照类型（与 protobuf 解耦，由 Publisher 序列化）。
 */

#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "shared_memory/ShmLayout.h"

namespace zrcs_nrt {

struct AxisStatusSnapshot {
    uint8_t axisId = 0;
    double position = 0.0;
    double cmdPosition = 0.0;
    double cmdVelocity = 0.0;
    double velocity = 0.0;
    double torque = 0.0;
};

struct AxisFeedbackFrameSnapshot {
    uint64_t sequence = 0;
    uint64_t simulationTimeNs = 0;
    std::vector<AxisStatusSnapshot> axes;
};

struct BtStatusSnapshot {
    std::string treeState;    ///< IDLE / LOADED / RUNNING / ...
    std::string currentNode;
    std::string message;
};

struct SystemStatusSnapshot {
    std::vector<AxisStatusSnapshot> axes;
    std::vector<AxisFeedbackFrameSnapshot> axisFeedbackFrames;
    std::string systemState;  ///< IDLE / RUN / STOP / ERROR / ...
    uint64_t heartbeat = 0;
    uint64_t droppedCommands = 0;
    BtStatusSnapshot bt;
    std::vector<zrcs::RtLogEntry> rtLogs;
    bool hasAxisFeedback = false;
};

} // namespace zrcs_nrt

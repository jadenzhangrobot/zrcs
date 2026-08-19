#pragma once
/**
 * @file StatusCollector.h
 * @brief 状态采集器 — 从各数据源收集原始快照，与序列化/发布分离。
 *
 * 职责边界：
 *  - StatusCollector: 只读各数据源 → 产出 SystemStatusSnapshot（纯数据）
 *  - StatusPublisher:  快照 → protobuf → ZMQ PUB（只负责传输）
 *
 * 这样可单独测试采集逻辑，更换序列化/传输协议不影响采集。
 */

#include "shared_memory/ShmLayout.h"
#include "status/StatusTypes.h"

#include <cstdint>
#include <string>
#include <vector>

class RtBridge;
class BehaviorTreeService;

/**
 * @brief 单次采集的状态快照（不含序列化/传输逻辑）。
 */
struct SystemStatusSnapshot {
    // 轴反馈帧
    struct AxisFrame {
        uint64_t sequence = 0;
        uint64_t simulationTimeNs = 0;
        uint32_t axisCount = 0;
        std::vector<double> position;
        std::vector<double> cmdPosition;
        std::vector<double> cmdVelocity;
        std::vector<double> velocity;
        std::vector<double> torque;
        std::vector<uint8_t> servoEnabled;
    };
    std::vector<AxisFrame> axisFrames;

    // 元数据
    uint64_t heartbeat = 0;
    uint64_t droppedCommands = 0;
    std::string systemState;

    // 行为树状态
    zrcs_nrt::BtStatus btStatus;

    // RT 日志
    std::vector<zrcs::RtLogEntry> rtLogs;
};

/**
 * @brief 采集器：从各数据源读取最新状态，产出快照。
 *
 * 线程安全：调用方负责保证传入的 RtBridge / BehaviorTreeService
 *           在采集期间有效。本类无内部状态，纯函数。
 */
class StatusCollector {
public:
    /// @param bridge        共享内存桥（读反馈帧、心跳、调度状态等）
    /// @param behaviorTree  行为树服务（读 BT 当前节点，可为 nullptr）
    explicit StatusCollector(RtBridge* bridge,
                             BehaviorTreeService* behaviorTree = nullptr);

    /// 采集一次完整快照。
    /// drainAxisFeedback: 是否搬空 SPSC 队列中的轴反馈帧；
    /// drainRtLogs: 是否搬空 RT 日志队列。
    SystemStatusSnapshot collect(bool drainAxisFeedback = true,
                                 bool drainRtLogs = true);

    /// 仅采集轴反馈帧列表（不采集元数据/日志）。
    static std::vector<SystemStatusSnapshot::AxisFrame>
    drainAxisFrames(RtBridge* bridge);

    /// 从 Bridge 原子读取心跳/调度/丢包等元数据。
    static void readMetadata(RtBridge* bridge,
                             uint64_t& heartbeat,
                             uint64_t& dropped,
                             std::string& systemState);

private:
    RtBridge* bridge_;
    BehaviorTreeService* behaviorTree_;
};

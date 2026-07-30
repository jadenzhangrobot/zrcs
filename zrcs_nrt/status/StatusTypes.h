#pragma once
/**
 * @file StatusTypes.h
 * @brief NRT 状态类型（与 protobuf 解耦，由 Publisher 序列化）。
 *
 * 这里定义的是 NRT 进程内部使用的纯 C++ 状态结构，刻意不引入 protobuf 类型。
 * 数据流向：
 *   RT 共享内存 (zrcs::AxisFeedbackData)
 *     -> StatusCollector 采集
 *     -> StatusStore 缓存（加锁）
 *     -> SystemStatus 快照
 *     -> StatusPublisher 转成 protobuf 并经 ZMQ 发给 GUI
 *
 * 之所以要单独一层类型，而不是直接把 protobuf 消息传来传去：
 * 1) StatusStore 持锁期间只做结构体拷贝，不碰 protobuf 的内存分配；
 * 2) protobuf 的 .proto 变更不会波及采集/缓存逻辑；
 * 3) 单元测试里构造这些结构比构造 protobuf 消息方便。
 */

#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "shared_memory/ShmLayout.h"

namespace zrcs_nrt {

/**
 * @brief 单个轴在某一拍的状态。
 *
 * 单位遵循控制器内部约定：直线轴为 m / m·s⁻¹，旋转轴为 rad / rad·s⁻¹
 * （注意不是 mm，G21 程序里的 mm 已在 NcParser 阶段换算过）。
 *
 * cmdPosition/cmdVelocity 是规划器下发的**命令**值，position/velocity 是
 * 驱动器回读的**实际**值。两者之差即跟随误差，排查轴抖动或超差时要对比看。
 */
struct AxisStatus {
    uint8_t axisId = 0;          ///< 轴号，对应 axis.xml 里的 axisId
    double position = 0.0;       ///< 实际位置（编码器反馈换算后）
    double cmdPosition = 0.0;    ///< 命令位置（规划器本拍输出）
    double cmdVelocity = 0.0;    ///< 命令速度（由命令位置差分得到）
    double velocity = 0.0;       ///< 实际速度
    double torque = 0.0;         ///< 实际力矩（未实现力矩读取的后端恒为 0）
};

/**
 * @brief 一拍完整的多轴反馈，带时序标识。
 *
 * RT 侧每个控制周期产生一帧。sequence 用来检测丢帧：NRT 采集慢于 RT 产生时，
 * 环形队列会覆盖旧帧，此时收到的 sequence 会出现跳跃。
 */
struct AxisFeedbackFrame {
    uint64_t sequence = 0;          ///< RT 侧递增帧号，用于丢帧检测
    uint64_t simulationTimeNs = 0;  ///< RT 侧时间戳（ns），仿真模式为仿真时间
    std::vector<AxisStatus> axes;  ///< 本帧内所有轴，下标即轴序
};

/**
 * @brief 行为树运行状态，供 GUI 显示当前执行到哪个节点。
 *
 * BehaviorTreeRunner 和 BehaviorTreeService 直接使用此类型，
 * 不再通过各自的内部 StatusSnapshot 中转。
 */
struct BtStatus {
    std::string treeState;    ///< IDLE / LOADED / RUNNING / SUCCESS / FAILURE / HALTED
    std::string currentNode;  ///< 当前正在 tick 的节点名
    std::string message;      ///< 附加信息，失败时为错误原因
};

/**
 * @brief 一次发布所需的全部状态。
 *
 * 由 StatusStore::snapshot() 生成。注意其中的"取出即清空"语义：
 * axisFeedbackFrames 和 rtLogs 是从待发布队列里搬出来的，同一份数据只会
 * 被拿到一次；systemState/heartbeat/bt 是当前值的拷贝，可以重复读到。
 */
struct SystemStatus {
    std::vector<AxisFeedbackFrame> axisFeedbackFrames;  ///< 本次待发布的所有帧（取出即清空）
    std::string systemState;      ///< IDLE / RUN / STOP / ERROR / RESET / SHUTDOWN
    uint64_t heartbeat = 0;       ///< RT 心跳计数，用于判断 RT 进程是否存活
    uint64_t droppedCommands = 0; ///< 命令队列满导致的丢弃累计数
    BtStatus bt;          ///< 行为树状态
    std::vector<zrcs::RtLogEntry> rtLogs;  ///< 本次待发布的 RT 日志（取出即清空）
};

} // namespace zrcs_nrt

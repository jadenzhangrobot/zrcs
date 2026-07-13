#pragma once

// RtBridge.h — NRT 侧共享内存统一访问层
//
// 所有 NRT 代码（ZMQ 线程、BT 引擎）对 SharedBlock 的访问都应通过本类完成。
// 线程安全：多个调用方可并发调用任意方法。
//   - cmdQueue 的单生产者语义由内部 push_mutex_ 保证（SPSC 要求单写者）。
//   - logQueue 的单消费者语义由 RtLogConsumer 独占保证。

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "shared_memory/ShmLayout.h"

class RtBridge {
public:
    enum class SendResult { OK, QUEUE_FULL, NOT_CONNECTED, UNKNOWN_CMD };

    struct CmdCompletion {
        uint32_t seq;
        bool     success;
    };

    explicit RtBridge(zrcs::SharedBlock* block);

    RtBridge(const RtBridge&) = delete;
    RtBridge& operator=(const RtBridge&) = delete;

    bool isConnected() const noexcept;

    // 1. 命令发送（NRT → RT cmdQueue）
    std::pair<SendResult, uint32_t> sendCommand(
        const std::string& name,
        const double* args = nullptr,
        size_t count       = 0);

    std::pair<SendResult, uint32_t> sendCommand(const std::string& name,
                                                 const std::vector<double>& args);

    // CSV 参数字符串便捷重载（BT 节点使用）
    std::pair<SendResult, uint32_t> sendCommand(const std::string& name,
                                                 const std::string& csv_args);

    // 2. 轴位置读取（RT → NRT，高频，LockFreeLatest）
    bool readLatestAxisPositions(zrcs::JointPosData& out) const noexcept;
    bool readLatestAxisFeedback(zrcs::AxisFeedbackData& out) noexcept;
    bool readMujocoIdentStatus(zrcs::MujocoIdentStatusData& out) const noexcept;
    uint8_t axisCount() const noexcept;

    // 3. 查询命令结果读取
    bool readFkResult(zrcs::FkResultData& out) const noexcept;
    bool readJointPosResult(zrcs::JointPosData& out) const noexcept;
    bool readProbeResult(zrcs::ProbeResultData& out) const noexcept;
    bool readCaptureResult(zrcs::CaptureData& out) const noexcept;
    bool probeTriggered() const noexcept;
    bool captureTriggered() const noexcept;
    uint32_t ioReadResult() const noexcept;

    // 4. 任务调度控制
    void setTaskScheduling(zrcs::TaskScheduling ts) noexcept;
    zrcs::TaskScheduling getTaskScheduling() const noexcept;
    void requestRun();
    void requestStop();
    void requestReset();
    void requestStart();
    void requestShutdown();

    // 5. 心跳监控
    uint64_t heartbeat() const noexcept;
    bool isRtAlive() noexcept;

    // 6. 速度倍率（0~100% → 0.0~1.0）
    void setSpeedMultiplier(uint8_t percent) noexcept;
    uint8_t speedMultiplier() const noexcept;

    // 7. 连续点动控制
    void startContinuousMotion(int axisId, bool direction) noexcept;
    void stopContinuousMotion() noexcept;

    // 8. 命令完成跟踪
    CmdCompletion lastCompletion() const noexcept;
    bool isCommandCompleted(uint32_t seq) const noexcept;
    bool waitForCompletion(
        uint32_t seq,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(10000)) noexcept;

    // 9. 配置标志
    void setConfJ(bool enabled) noexcept;
    void setConfL(bool enabled) noexcept;
    void setSingAreaMode(uint8_t mode) noexcept;

    // 10. MoveL / MoveLGalvo 运动限制配置
    void setPathMoveConfig(double maxVel, double maxAccel, double maxJerk) noexcept;
    void setGalvoConfig(int platXId, int platYId, int galvoXId, int galvoYId,
                        double cutoffHz) noexcept;

    // 11. 诊断
    uint64_t droppedCount() const noexcept;

private:
    zrcs::SharedBlock*                                                  block_;
    zrcs::ShmSPSCProducer<zrcs::Command, zrcs::kCmdQueueCap>          cmdProducer_;
    zrcs::ShmSPSCConsumer<zrcs::AxisFeedbackData, zrcs::kLogQueueCap> axisFbConsumer_;
    std::mutex                                                          push_mutex_;
    std::atomic<uint32_t>                                               seq_counter_{1};
    std::atomic<uint64_t>                                               dropped_count_{0};
    uint64_t                                                            prev_heartbeat_{0};
};

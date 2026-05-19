#pragma once

// RtBridge.h — NRT 侧共享内存统一访问层
//
// 所有 NRT 代码（ZMQ 线程、BT 引擎）对 SharedBlock 的访问都应通过本类完成。
// 线程安全：多个调用方可并发调用任意方法。
//   - cmdQueue 的单生产者语义由内部 push_mutex_ 保证（SPSC 要求单写者）。
//   - logQueue 的单消费者语义由 RtLogConsumer 独占保证。

#include <atomic>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <chrono>
#include <mutex>
#include <thread>
#include <algorithm>
#include <optional>
#include <spdlog/spdlog.h>
#include "shared_memory/ShmLayout.h"
#include "config/CmdDefine.h"
class RtBridge {
public:
    enum class SendResult { OK, QUEUE_FULL, NOT_CONNECTED, UNKNOWN_CMD };

    struct CmdCompletion {
        uint32_t seq;
        bool     success;
    };

    explicit RtBridge(zrcs::SharedBlock* block)
        : block_(block),
          cmdProducer_(block->cmdQueue),
          axisFbConsumer_(block->axisFeedbackQueue),
          pathProducer_(block->pathQueue)
    {}

    RtBridge(const RtBridge&) = delete;
    RtBridge& operator=(const RtBridge&) = delete;

    bool isConnected() const noexcept { return block_ != nullptr; }

    // ─────────────────────────────────────────────────────────────────
    // 1. 命令发送（NRT → RT cmdQueue）
    // ─────────────────────────────────────────────────────────────────

    std::pair<SendResult, uint32_t> sendCommand(
        const std::string& name,
        const double* args = nullptr,
        size_t count       = 0)
    {
        if (!block_) return {SendResult::NOT_CONNECTED, 0};

        const auto cmdId = zrcs::cmdNameToId(name);
        if (!cmdId.has_value() || *cmdId == CmdId::INVALID)
        {
            spdlog::error("[RtBridge] Unknown command '{}', not registered in cmdNameToId", name);
            return {SendResult::UNKNOWN_CMD, 0};
        }

        zrcs::Command cmd{};
        cmd.cmdId = static_cast<uint16_t>(*cmdId);
        cmd.seq   = seq_counter_.fetch_add(1, std::memory_order_relaxed);

        const size_t n = std::min(count, zrcs::kCmdArgsMax);
        if (args && n > 0) std::memcpy(cmd.args, args, n * sizeof(double));
        {
            std::lock_guard<std::mutex> lk(push_mutex_);
            if (!cmdProducer_.push(cmd)) 
            {
                ++dropped_count_;
                spdlog::error("[RtBridge] Queue full, dropped '{}' (seq={})", name, cmd.seq);
                return {SendResult::QUEUE_FULL, 0};
            }
        }

        spdlog::info("[RtBridge] Sent '{}' seq={} cmdId={} head_after={}",
                     name, cmd.seq, cmd.cmdId,
                     block_->cmdQueue.head.load(std::memory_order_relaxed));
        return {SendResult::OK, cmd.seq};
    }

    std::pair<SendResult, uint32_t> sendCommand( const std::string& name, const std::vector<double>& args)
    {
        return sendCommand(name, args.data(), args.size());
    }

    // CSV 参数字符串便捷重载（BT 节点使用）
    std::pair<SendResult, uint32_t> sendCommand(const std::string& name, const std::string& csv_args)
    {
        if (csv_args.empty()) return sendCommand(name, nullptr, 0);

        double buf[zrcs::kCmdArgsMax]{};
        size_t idx = 0;
        size_t pos = 0;
        while (idx < zrcs::kCmdArgsMax) 
        {
            size_t comma = csv_args.find(',', pos);
            const std::string token = csv_args.substr(pos, comma - pos);
            try { buf[idx++] = std::stod(token); } catch (...) { break; }
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
        return sendCommand(name, buf, idx);
    }

    // ─────────────────────────────────────────────────────────────────
    // 2. 轴位置读取（RT → NRT，高频，LockFreeLatest）
    // ─────────────────────────────────────────────────────────────────

    bool readLatestAxisPositions(zrcs::JointPosData& out) const noexcept 
    {
        if (!block_) return false;
        return zrcs::lfl_read(block_->axisPositions, out);
    }

    /// 从 axisFeedbackQueue 逐帧读取一条完整轴数据，不主动丢弃中间帧
    bool readLatestAxisFeedback(zrcs::AxisFeedbackData& out) noexcept 
    {
        return axisFbConsumer_.pop(out);
    }

    uint8_t axisCount() const noexcept 
    {
        if (!block_) return 0;
        return block_->axisCount.load(std::memory_order_acquire);
    }

    // ─────────────────────────────────────────────────────────────────
    // 3. 查询命令结果读取（修复旧版裸数组数据竞争）
    // ────────────────────────────────────────────────────────────────

    bool readFkResult(zrcs::FkResultData& out) const noexcept {
        if (!block_) return false;
        return zrcs::lfl_read(block_->fkResult, out);
    }

    bool readJointPosResult(zrcs::JointPosData& out) const noexcept 
    {
        if (!block_) return false;
        return zrcs::lfl_read(block_->jointPosResult, out);
    }

    bool readProbeResult(zrcs::ProbeResultData& out) const noexcept 
    {
        if (!block_) return false;
        return zrcs::lfl_read(block_->probeResult, out);
    }

    bool readCaptureResult(zrcs::CaptureData& out) const noexcept {
        if (!block_) return false;
        return zrcs::lfl_read(block_->captureResult, out);
    }

    bool probeTriggered() const noexcept 
    {
        if (!block_) return false;
        return block_->probeTriggered.load(std::memory_order_acquire);
    }

    bool captureTriggered() const noexcept 
    {
        if (!block_) return false;
        return block_->captureTriggered.load(std::memory_order_acquire);
    }

    uint32_t ioReadResult() const noexcept 
    {
        if (!block_) return 0;
        return block_->ioReadResult.load(std::memory_order_acquire);
    }

    // ─────────────────────────────────────────────────────────────────
    // 4. 任务调度控制
    // ─────────────────────────────────────────────────────────────────

    void setTaskScheduling(zrcs::TaskScheduling ts) noexcept 
    {
        if (block_) block_->taskSched.store(ts, std::memory_order_release);
    }

    zrcs::TaskScheduling getTaskScheduling() const noexcept 
    {
        if (!block_) return zrcs::TaskScheduling::STOP;
        return block_->taskSched.load(std::memory_order_acquire);
    }

    void requestRun()      { setTaskScheduling(zrcs::TaskScheduling::RUN); }
    void requestStop()
    {
        stopContinuousMotion();
        setTaskScheduling(zrcs::TaskScheduling::STOP);
    }
    void requestReset()
    {
        stopContinuousMotion();
        setTaskScheduling(zrcs::TaskScheduling::RESET);
    }
    void requestStart()    { setTaskScheduling(zrcs::TaskScheduling::IDLE);}
    void requestShutdown() { setTaskScheduling(zrcs::TaskScheduling::SHUTDOWN); }

    // ─────────────────────────────────────────────────────────────────
    // 5. 心跳监控
    // ─────────────────────────────────────────────────────────────────

    uint64_t heartbeat() const noexcept 
    {
        if (!block_) return 0;
        uint64_t value = 0;
        zrcs::lfl_read(block_->heartbeat, value);
        return value;
    }

    bool isRtAlive() noexcept
    {
        const uint64_t cur = heartbeat();
        const bool alive = (cur != prev_heartbeat_);
        prev_heartbeat_ = cur;
        return alive;
    }

    // ─────────────────────────────────────────────────────────────────
    // 6. 速度倍率（0~100% → 0.0~1.0）
    // ─────────────────────────────────────���───────────────────────────

    void setSpeedMultiplier(uint8_t percent) noexcept 
    {
        if (!block_) return;
        block_->overrideRatio.store(
            std::clamp(percent / 100.0, 0.0, 1.0), std::memory_order_release);
    }

    uint8_t speedMultiplier() const noexcept
    {
        if (!block_) return 0;
        return static_cast<uint8_t>(
            block_->overrideRatio.load(std::memory_order_acquire) * 100.0);
     }

    // ─────────────────────────────────────────────────────────────────
    // 7. 连续点动控制
    // ─────────────────────────────────────────────────────────────────

    void startContinuousMotion(int axisId, bool direction) noexcept 
    {
        if (!block_) return;
        block_->jogCtrl.axisId.store(axisId, std::memory_order_relaxed);
        block_->jogCtrl.direction.store(direction, std::memory_order_relaxed);
        block_->jogCtrl.active.store(true, std::memory_order_release);
    }

    void stopContinuousMotion() noexcept
    {
        if (block_) block_->jogCtrl.active.store(false, std::memory_order_release);
    }

    // ─────────────────────────────────────────────────────────────────
    // 8. 命令完成跟踪
    // ─────────────────────────────────────────────────────────────────

    CmdCompletion lastCompletion() const noexcept 
    {
        if (!block_) return {0, false};
        const auto completion = zrcs::unpackCmdCompletion(
            block_->lastCmdCompletion.load(std::memory_order_acquire));
        return {completion.seq, completion.result == 0};
    }

    bool isCommandCompleted(uint32_t seq) const noexcept {
        if (!block_) return false;
        const auto completion = zrcs::unpackCmdCompletion(
            block_->lastCmdCompletion.load(std::memory_order_acquire));
        return completion.seq >= seq;
    }

    bool waitForCompletion(uint32_t seq,std::chrono::milliseconds timeout = std::chrono::milliseconds(10000)) noexcept
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (isCommandCompleted(seq)) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    // ─────────────────────────────────────────────────────────────────
    // 9. 配置标志
    // ─────────────────────────────────────────────────────────────────

    void setConfJ(bool enabled) noexcept
    {
        if (block_) block_->confJEnabled.store(enabled, std::memory_order_release);
    }
    void setConfL(bool enabled) noexcept
    {
        if (block_) block_->confLEnabled.store(enabled, std::memory_order_release);
    }
    void setSingAreaMode(uint8_t mode) noexcept 
    {
        if (block_) block_->singAreaMode.store(mode, std::memory_order_release);
    }

    // ─────────────────────────────────────────────────────────────────
    // 10. 路径运动（NRT → RT pathQueue）
    // ─────────────────────────────────────────────────────────────────

    bool pushPathPoint(const zrcs::PathPoint& pt) noexcept 
    {
        if (!block_) return false;
        return pathProducer_.push(pt);
    }

    void setPathMoveConfig(double maxVel, double maxAccel, double maxJerk) noexcept 
    {
        if (!block_) return;
        block_->pathMoveCfg.maxVel.store(maxVel, std::memory_order_release);
        block_->pathMoveCfg.maxAccel.store(maxAccel, std::memory_order_release);
        block_->pathMoveCfg.maxJerk.store(maxJerk, std::memory_order_release);
    }

    void setGalvoConfig(int platXId, int platYId, int galvoXId, int galvoYId, double cutoffHz) noexcept 
    {
        if (!block_) return;
        block_->galvoCfg.platXId.store(platXId, std::memory_order_release);
        block_->galvoCfg.platYId.store(platYId, std::memory_order_release);
        block_->galvoCfg.galvoXId.store(galvoXId, std::memory_order_release);
        block_->galvoCfg.galvoYId.store(galvoYId, std::memory_order_release);
        block_->galvoCfg.cutoffHz.store(cutoffHz, std::memory_order_release);
    }

    void setPathMoveActive(bool active) noexcept 
    {
        if (!block_) return;
        block_->pathMoveActive.store(active, std::memory_order_release);
    }

    // ─────────────────────────────────────────────────────────────────
    // 11. 诊断
    // ─────────────────────────────────────────────────────────────────

    uint64_t droppedCount() const noexcept 
    {
        return dropped_count_.load(std::memory_order_relaxed);
    }

    // 命令名称 → CmdId 映射表由 CmdDefine.h 提供
    // 使用 zrcs::cmdNameToId() 获取

private:
    zrcs::SharedBlock*                                                         block_;
    zrcs::ShmSPSCProducer<zrcs::Command, zrcs::kCmdQueueCap>                 cmdProducer_;
    zrcs::ShmSPSCConsumer<zrcs::AxisFeedbackData, zrcs::kLogQueueCap>        axisFbConsumer_;
    zrcs::ShmSPSCProducer<zrcs::PathPoint, zrcs::kPathBufCap>               pathProducer_;
    std::mutex                                                                push_mutex_;
    std::atomic<uint32_t>                                                     seq_counter_{1};
    std::atomic<uint64_t>                                                     dropped_count_{0};
    uint64_t                                                                  prev_heartbeat_{0};
};

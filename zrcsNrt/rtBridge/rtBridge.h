#pragma once

#include <atomic>
#include <array>
#include <string>
#include <vector>
#include <cstring>
#include <chrono>
#include <mutex>
#include <thread>
#include <sstream>
#include <spdlog/spdlog.h>
#include "sharedMemory/sharedData.h"
#include "config/parameter.h"

/**
 * @brief NRT→RT 共享内存通信的统一封装层
 *
 * 所有 NRT 侧对 SharedBlock 的访问都应通过本类完成。
 * 线程安全：多个调用方（ZMQ 线程、BT 线程、终端线程）可并发调用任意方法。
 * commandQueue 的单生产者语义由内部 push_mutex_ 保证。
 */
class RtBridge {
public:
    enum class SendResult {
        OK,
        QUEUE_FULL,
        NOT_CONNECTED
    };

    struct CmdCompletion {
        uint32_t seq;
        bool     success;  // true = result==0
    };

    explicit RtBridge(SharedBlock* block)
        : block_(block) {}

    RtBridge(const RtBridge&) = delete;
    RtBridge& operator=(const RtBridge&) = delete;

    bool isConnected() const { return block_ != nullptr; }

    // =================================================================
    // 1. 命令发送 (NRT → RT commandQueue)
    // =================================================================

    /**
     * @brief 发送命名命令（核心重载）
     * @param name  命令名（如 "MoveJ"、"Enable"），截断到 MAX_CMD_NAME-1
     * @param args  参数数组指针（count==0 时可为 nullptr）
     * @param count 参数个数（上限 MAX_CMD_ARGS）
     * @return {SendResult, 分配的序列号}，失败时 seq 为 0
     */
    std::pair<SendResult, uint32_t> sendCommand(
        const std::string& name,
        const double* args = nullptr,
        size_t count = 0)
    {
        if (!block_) return {SendResult::NOT_CONNECTED, 0};

        Command shm_cmd{};
        std::strncpy(shm_cmd.cmd, name.c_str(), MAX_CMD_NAME - 1);
        shm_cmd.cmd[MAX_CMD_NAME - 1] = '\0';

        size_t n = std::min(count, static_cast<size_t>(MAX_CMD_ARGS));
        if (args && n > 0) {
            std::memcpy(shm_cmd.args, args, n * sizeof(double));
        }

        uint32_t seq = seq_counter_.fetch_add(1, std::memory_order_relaxed);
        shm_cmd.seq = seq;

        {
            std::lock_guard<std::mutex> lock(push_mutex_);
            if (!block_->commandQueue.push(shm_cmd)) {
                ++dropped_count_;
                spdlog::error("[RtBridge] Queue full, dropped '{}' (seq={})", name, seq);
                return {SendResult::QUEUE_FULL, 0};
            }
        }

        spdlog::debug("[RtBridge] Sent '{}' seq={} args_count={}", name, seq, n);
        return {SendResult::OK, seq};
    }

    std::pair<SendResult, uint32_t> sendCommand(
        const std::string& name,
        const std::vector<double>& args)
    {
        return sendCommand(name, args.data(), args.size());
    }

    /**
     * @brief 逗号分隔参数字符串的便捷重载（BT 节点使用）
     */
    std::pair<SendResult, uint32_t> sendCommand(
        const std::string& name,
        const std::string& csv_args)
    {
        if (csv_args.empty()) {
            return sendCommand(name, nullptr, 0);
        }

        double buf[MAX_CMD_ARGS]{};
        size_t idx = 0;
        std::istringstream ss(csv_args);
        std::string token;
        while (std::getline(ss, token, ',') && idx < MAX_CMD_ARGS) {
            try {
                buf[idx++] = std::stod(token);
            } catch (...) {
                break;
            }
        }
        return sendCommand(name, buf, idx);
    }

    // =================================================================
    // 2. 状态读取 (RT → NRT statusQueue)
    // =================================================================

    bool readAxisPositions(std::array<double, AXISMAXCOUNT>& out) {
        if (!block_) return false;
        return block_->statusQueue.pop(out);
    }

    /**
     * @brief 排空状态队列，只保留最新一帧（消费者较慢时使用）
     */
    bool readLatestAxisPositions(std::array<double, AXISMAXCOUNT>& out) {
        if (!block_) return false;
        bool got_any = false;
        std::array<double, AXISMAXCOUNT> temp{};
        while (block_->statusQueue.pop(temp)) {
            out = temp;
            got_any = true;
        }
        return got_any;
    }

    uint8_t axisCount() const {
        if (!block_) return 0;
        return block_->axisCount.load(std::memory_order_acquire);
    }

    // =================================================================
    // 3. 任务调度控制
    // =================================================================

    void setTaskScheduling(TaskScheduling ts) {
        if (block_) block_->cmd.store(ts, std::memory_order_release);
    }

    TaskScheduling getTaskScheduling() const {
        if (!block_) return TaskScheduling::STOP;
        return block_->cmd.load(std::memory_order_acquire);
    }

    void requestRun()      { setTaskScheduling(TaskScheduling::RUN); }
    void requestStop()     { setTaskScheduling(TaskScheduling::STOP); }
    void requestReset()    { setTaskScheduling(TaskScheduling::RESET); }
    void requestStart()    { setTaskScheduling(TaskScheduling::START); }
    void requestShutdown() { setTaskScheduling(TaskScheduling::SHUTDOWN); }

    // =================================================================
    // 4. 心跳监控
    // =================================================================

    uint64_t heartBeat() const {
        if (!block_) return 0;
        return block_->heartBeat.load(std::memory_order_acquire);
    }

    /**
     * @brief 检查 RT 心跳是否比上次调用时有推进（RT 可能冻结时返回 false）
     */
    bool isRtAlive() {
        uint64_t current = heartBeat();
        bool alive = (current != prev_heartbeat_);
        prev_heartbeat_ = current;
        return alive;
    }

    // =================================================================
    // 5. 倍率控制 (0-100%)
    // =================================================================

    void setSpeedMultiplier(uint8_t percent) {
        if (block_) block_->Multiplied.store(percent, std::memory_order_release);
    }

    uint8_t speedMultiplier() const {
        if (!block_) return 0;
        return block_->Multiplied.load(std::memory_order_acquire);
    }

    // =================================================================
    // 6. 单轴连续运动控制
    // =================================================================

    void startContinuousMotion(int axisId, bool direction) {
        if (!block_) return;
        block_->sacm.axisId.store(axisId, std::memory_order_relaxed);
        block_->sacm.direction.store(direction, std::memory_order_relaxed);
        block_->sacm.motion.store(true, std::memory_order_release);
    }

    void stopContinuousMotion() {
        if (!block_) return;
        block_->sacm.motion.store(false, std::memory_order_release);
    }

    // =================================================================
    // 7. 命令完成跟踪
    // =================================================================

    CmdCompletion lastCompletion() const {
        if (!block_) return {0, false};
        uint32_t seq = block_->lastCmdSeq.load(std::memory_order_acquire);
        uint8_t  res = block_->lastCmdResult.load(std::memory_order_acquire);
        return {seq, res == 0};
    }

    bool isCommandCompleted(uint32_t seq) const {
        if (!block_) return false;
        return block_->lastCmdSeq.load(std::memory_order_acquire) >= seq;
    }

    /**
     * @brief 阻塞等待指定序列号命令完成
     * @return true 如果在超时内完成
     */
    bool waitForCompletion(
        uint32_t seq,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(10000))
    {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (isCommandCompleted(seq)) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    // =================================================================
    // 8. 诊断
    // =================================================================

    uint64_t droppedCount() const { return dropped_count_.load(std::memory_order_relaxed); }

private:
    SharedBlock*          block_;
    std::atomic<uint32_t> seq_counter_{1};
    std::atomic<uint64_t> dropped_count_{0};
    uint64_t              prev_heartbeat_{0};
    std::mutex            push_mutex_;
};

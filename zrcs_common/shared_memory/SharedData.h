#pragma once

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "config/CmdArgs.h"
#include "ShmConstants.h"

// ===================================================================
// 1. 定义要在进程间传递的数据结构
// ===================================================================
constexpr size_t OUTPUTIOSIZE = 32;
constexpr size_t INPUTIOSIZE  = 32;

constexpr size_t COMMAND_BUFFER_SIZE = 64;   // 扩大：16 -> 64，防止高频场景丢命令
constexpr size_t STATUS_BUFFER_SIZE  = 64;   // 同步扩大
constexpr size_t MAX_CMD_NAME        = 100;  // 命令名最大长度
constexpr size_t MAX_CMD_ARGS        = 20;   // 参数最大个数：10 -> 20，满足复杂轨迹命令
constexpr size_t LOG_BUFFER_SIZE     = 256;  // RT日志环形队列容量 (必须为2的幂)

// ===================================================================
// 2. SPSC 无锁环形队列（模板，实现必须在头文件）
// ===================================================================
template <typename T, size_t Capacity>
class SPSCRingBuffer {
private:
    static constexpr size_t MASK = Capacity - 1;
    static_assert((Capacity > 0) && ((Capacity & (Capacity - 1)) == 0),
                  "Capacity must be a power of 2");

    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};

    size_t cached_tail_{0};
    size_t cached_head_{0};

    std::array<T, Capacity> buffer_;

public:
    SPSCRingBuffer() = default;

    bool push(const T& item)
    {
        const auto current_head = head_.load(std::memory_order_relaxed);
        const auto next_head = (current_head + 1) & MASK;

        if (next_head == cached_tail_) {
            cached_tail_ = tail_.load(std::memory_order_acquire);
            if (next_head == cached_tail_) {
                return false;
            }
        }

        buffer_[current_head] = item;
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    bool pop(T& item)
    {
        const auto current_tail = tail_.load(std::memory_order_relaxed);

        if (current_tail == cached_head_) {
            cached_head_ = head_.load(std::memory_order_acquire);
            if (current_tail == cached_head_) {
                return false;
            }
        }

        item = buffer_[current_tail];
        tail_.store((current_tail + 1) & MASK, std::memory_order_release);
        return true;
    }

    size_t size() const {
        auto h = head_.load(std::memory_order_acquire);
        auto t = tail_.load(std::memory_order_acquire);
        return (h >= t) ? (h - t) : (Capacity - t + h);
    }
};

// ===================================================================
// 3. 数据结构定义
// ===================================================================

// RT -> NRT 日志传输条目 (固定大小, 无动态分配)
struct RtLogEntry {
    uint64_t timestamp_us;   // 微秒级单调时钟时间戳
    uint8_t  level;          // 0=INFO, 1=WARN, 2=ERROR
    uint8_t  padding[1];     // 对齐填充
    uint16_t line;           // 源文件行号
    char     file[60];       // 源文件名 (截断)
    char     message[192];   // 格式化消息 (截断)
};

struct singleAxisContinueMotion
{
    std::atomic<int>  axisId{0};
    std::atomic<bool> motion{false};
    std::atomic<bool> direction{true};
};

static_assert(ATOMIC_INT_LOCK_FREE  == 2, "atomic<int> must be lock-free for cross-process safety");
static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "atomic<bool> must be lock-free for cross-process safety");
static_assert(ATOMIC_LLONG_LOCK_FREE == 2, "atomic<uint64_t> must be lock-free for cross-process safety");
static_assert(ATOMIC_CHAR_LOCK_FREE == 2, "atomic<uint8_t> must be lock-free for cross-process safety");

struct Command
{
    double   args[MAX_CMD_ARGS] = {0};
    uint32_t seq = 0;
    char     cmd[MAX_CMD_NAME] = {0};
};

struct SharedBlock {
    SPSCRingBuffer<Command, COMMAND_BUFFER_SIZE>                          commandQueue;
    SPSCRingBuffer<std::array<double, AxisMaxCount>, STATUS_BUFFER_SIZE>  statusQueue;
    SPSCRingBuffer<RtLogEntry, LOG_BUFFER_SIZE>                           logQueue;

    std::atomic<TaskScheduling> cmd;
    std::atomic<uint64_t>       heartBeat;
    std::atomic<uint8_t>        Multiplied;
    std::atomic<uint8_t>        axisCount;

    singleAxisContinueMotion sacm;

    std::atomic<uint32_t> lastCmdSeq{0};
    std::atomic<uint8_t>  lastCmdResult{0};

    std::atomic<double> overrideRatio{1.0};

    double fkResult[6] = {0};
    double jointPosResult[AxisMaxCount] = {0};

    std::atomic<uint32_t> ioReadResult{0};

    double probeResult[6] = {0};
    std::atomic<bool> probeTriggered{false};

    double capturedPos[AxisMaxCount] = {0};
    std::atomic<bool> captureTriggered{false};

    std::atomic<bool>    confJEnabled{true};
    std::atomic<bool>    confLEnabled{true};
    std::atomic<uint8_t> singAreaMode{0};
};

// ===================================================================
// 4. 类型安全的共享内存访问器
// ===================================================================
class ShmAccessor {
public:
    explicit ShmAccessor(SharedBlock* blk);

    std::atomic<TaskScheduling>&  taskScheduling();
    SPSCRingBuffer<Command, COMMAND_BUFFER_SIZE>& cmdQueue();
    SPSCRingBuffer<std::array<double, AxisMaxCount>, STATUS_BUFFER_SIZE>& statusQueue();
    SPSCRingBuffer<RtLogEntry, LOG_BUFFER_SIZE>& logQueue();
    std::atomic<uint64_t>&  heartBeat();
    std::atomic<uint8_t>&   axisCount();
    std::atomic<uint8_t>&   multiPlied();
    singleAxisContinueMotion& continueMotion();
    std::atomic<uint32_t>&  lastCmdSeq();
    std::atomic<uint8_t>&   lastCmdResult();
    std::atomic<double>&    overrideRatio();
    double*                 fkResult();
    double*                 jointPosResult();
    std::atomic<uint32_t>&  ioReadResult();
    double*                 probeResult();
    std::atomic<bool>&      probeTriggered();
    double*                 capturedPos();
    std::atomic<bool>&      captureTriggered();
    std::atomic<bool>&      confJEnabled();
    std::atomic<bool>&      confLEnabled();
    std::atomic<uint8_t>&   singAreaMode();

private:
    SharedBlock* blk_;
};

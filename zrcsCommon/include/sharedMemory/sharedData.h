#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "config/parameter.h"
#include "config/cmdArgs.h"




// ===================================================================
// 1. 定义要在进程间传递的数据结构
// ===================================================================
constexpr size_t OUTPUTIOSIZE = 32;
constexpr size_t INPUTIOSIZE  = 32;

constexpr size_t COMMAND_BUFFER_SIZE = 64;   // 扩大：16 -> 64，防止高频场景丢命令
constexpr size_t STATUS_BUFFER_SIZE = 64;    // 同步扩大

constexpr size_t MAX_CMD_NAME  = 100;        // 命令名最大长度
constexpr size_t MAX_CMD_ARGS  = 20;         // 参数最大个数：10 -> 20，满足复杂轨迹命令

// 命令的类型

template <typename T, size_t Capacity>
class SPSCRingBuffer {
private:
    static constexpr size_t MASK = Capacity - 1;  // 位掩码，替代取模运算
    static_assert((Capacity > 0) && ((Capacity & (Capacity - 1)) == 0),
                  "Capacity must be a power of 2");

    // head: 由生产者修改，指向下一个可写入的位置
    // tail: 由消费者修改，指向下一个可读取的位置
    // 当 head == tail 时，缓冲区为空
    // 当 (head + 1) & MASK == tail 时，缓冲区为满
    alignas(64) std::atomic<size_t> head_{0}; // 64字节对齐避免伪共享
    alignas(64) std::atomic<size_t> tail_{0};

    // 本地缓存对端游标，减少跨核缓存一致性流量
    size_t cached_tail_{0};  // 生产者缓存的 tail 值
    size_t cached_head_{0};  // 消费者缓存的 head 值

    std::array<T, Capacity> buffer_;

public:
    SPSCRingBuffer() = default;

    // 由生产者调用
    bool push(const T& item)
    {
        const auto current_head = head_.load(std::memory_order_relaxed);
        const auto next_head = (current_head + 1) & MASK;

        // 先用本地缓存的 tail 判断，避免每次都读远端原子变量
        if (next_head == cached_tail_) {
            cached_tail_ = tail_.load(std::memory_order_acquire);
            if (next_head == cached_tail_) {
                return false; // 缓冲区确实已满
            }
        }

        buffer_[current_head] = item;

        // 更新 head，使用 release 语义，确保数据写入对消费者可见
        head_.store(next_head, std::memory_order_release);
        return true;
    }

    // 由消费者调用
    bool pop(T& item)
    {
        const auto current_tail = tail_.load(std::memory_order_relaxed);

        // 先用本地缓存的 head 判断，避免每次都读远端原子变量
        if (current_tail == cached_head_) {
            cached_head_ = head_.load(std::memory_order_acquire);
            if (current_tail == cached_head_) {
                return false; // 缓冲区确实为空
            }
        }

        item = buffer_[current_tail];

        // 更新 tail，使用 release 语义，告知生产者一个槽位已空出
        tail_.store((current_tail + 1) & MASK, std::memory_order_release);
        return true;
    }

    // 返回当前缓冲区中的元素数量（近似值，用于监控）
    size_t size() const {
        auto h = head_.load(std::memory_order_acquire);
        auto t = tail_.load(std::memory_order_acquire);
        return (h >= t) ? (h - t) : (Capacity - t + h);
    }
};



struct singleAxisContinueMotion
{
   std::atomic<int>  axisId{0};
   std::atomic<bool> motion{false};
   std::atomic<bool> direction{true};
};
static_assert(ATOMIC_INT_LOCK_FREE == 2,
              "atomic<int> must be lock-free for cross-process safety");
static_assert(ATOMIC_BOOL_LOCK_FREE == 2,
              "atomic<bool> must be lock-free for cross-process safety");
static_assert(ATOMIC_LLONG_LOCK_FREE == 2,
              "atomic<uint64_t> must be lock-free for cross-process safety");
static_assert(ATOMIC_CHAR_LOCK_FREE == 2,
              "atomic<uint8_t> must be lock-free for cross-process safety");
struct Command
{
    double   args[MAX_CMD_ARGS] = {0};  // 8字节对齐，放首位消除 padding
    uint32_t seq = 0;                   // 命令序列号，用于 NRT 跟踪完成状态
    char     cmd[MAX_CMD_NAME] = {0};
};

struct SharedBlock {
    // NRT -> RT 的命令通道
    SPSCRingBuffer<Command, COMMAND_BUFFER_SIZE> commandQueue;
    // RT -> NRT 的状态通道
    SPSCRingBuffer<std::array<double, AXISMAXCOUNT>, STATUS_BUFFER_SIZE> statusQueue;
    std::atomic<TaskScheduling>  cmd;
    std::atomic<uint64_t>  heartBeat; //心跳
    std::atomic<uint8_t>   Multiplied;//倍率
    std::atomic<uint8_t>   axisCount;//轴数量

    singleAxisContinueMotion sacm;

    // RT -> NRT 命令完成反馈
    std::atomic<uint32_t> lastCmdSeq{0};     // RT 最后完成的命令序列号
    std::atomic<uint8_t>  lastCmdResult{0};  // 0=成功, 1=失败
};

// 类型安全的共享内存访问器，替代原有的 #define 宏
class ShmAccessor {
public:
    explicit ShmAccessor(SharedBlock* blk) : blk_(blk) {}

    std::atomic<TaskScheduling>&  taskScheduling()  { return blk_->cmd; }
    SPSCRingBuffer<Command, COMMAND_BUFFER_SIZE>& cmdQueue() { return blk_->commandQueue; }
    SPSCRingBuffer<std::array<double, AXISMAXCOUNT>, STATUS_BUFFER_SIZE>& statusQueue() { return blk_->statusQueue; }
    std::atomic<uint64_t>&  heartBeat()       { return blk_->heartBeat; }
    std::atomic<uint8_t>&   axisCount()       { return blk_->axisCount; }
    std::atomic<uint8_t>&   multiPlied()      { return blk_->Multiplied; }
    singleAxisContinueMotion& continueMotion() { return blk_->sacm; }
    std::atomic<uint32_t>&  lastCmdSeq()      { return blk_->lastCmdSeq; }
    std::atomic<uint8_t>&   lastCmdResult()   { return blk_->lastCmdResult; }

private:
    SharedBlock* blk_;
};

#endif // SHARED_DATA_HPP
#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <sys/types.h>
#include "config/parameter.h"
#include "config/cmdArgs.h"




// ===================================================================
// 1. 定义要在进程间传递的数据结构
// ===================================================================
#define  OUTPUTIOSIZE 32
#define  INPUTIOSIZE  32

constexpr size_t COMMAND_BUFFER_SIZE = 64;   // 扩大：16 -> 64，防止高频场景丢命令
constexpr size_t STATUS_BUFFER_SIZE = 64;    // 同步扩大

constexpr size_t MAX_CMD_NAME  = 100;        // 命令名最大长度
constexpr size_t MAX_CMD_ARGS  = 20;         // 参数最大个数：10 -> 20，满足复杂轨迹命令

// 命令的类型

template <typename T, size_t Capacity>
class SPSCRingBuffer {
private:
    // 确保缓冲区容量是2的幂，可以简化取模运算为位运算，但这里用%以保证通用性
    static_assert((Capacity > 0) && ((Capacity & (Capacity - 1)) == 0),
                  "Capacity must be a power of 2");

    // head: 由生产者修改，指向下一个可写入的位置
    // tail: 由消费者修改，指向下一个可读取的位置
    // 当 head == tail 时，缓冲区为空
    // 当 (head + 1) % Capacity == tail 时，缓冲区为满
    alignas(64) std::atomic<size_t> head_{0}; // 64字节对齐避免伪共享
    alignas(64) std::atomic<size_t> tail_{0};

    std::array<T, Capacity> buffer_;

public:
    SPSCRingBuffer() = default;

    // 由生产者调用
    bool push(const T& item)
    {
        const auto current_head = head_.load(std::memory_order_relaxed);
        const auto next_head = (current_head + 1) % Capacity;

        // 检查缓冲区是否已满。加载 tail 时使用 acquire 语义，
        // 确保能看到消费者对 tail 的最新更新。
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false; // 缓冲区已满
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

        // 检查缓冲区是否为空。加载 head 时使用 acquire 语义，
        // 确保能看到生产者写入的数据和对 head 的更新。
        if (current_tail == head_.load(std::memory_order_acquire)) {
            return false; // 缓冲区为空
        }

        item = buffer_[current_tail];

        // 更新 tail，使用 release 语义，告知生产者一个槽位已空出
        tail_.store((current_tail + 1) % Capacity, std::memory_order_release);
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
   int axisId=0;
   bool motion=false;
   bool direction=true;
};
struct Command
{
    char     cmd[MAX_CMD_NAME] = {0};
    double   args[MAX_CMD_ARGS] = {0};
    uint32_t seq = 0;               // 命令序列号，用于 NRT 跟踪完成状态
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
    std::array<double, AXISMAXCOUNT>  axisPosition;
    std::array<double, AXISMAXCOUNT>  axisVelocity;


    std::atomic<singleAxisContinueMotion> sacm;

    // RT -> NRT 命令完成反馈
    std::atomic<uint32_t> lastCmdSeq{0};     // RT 最后完成的命令序列号
    std::atomic<uint8_t>  lastCmdResult{0};  // 0=成功, 1=失败
};
#define taskScheduling rtProcess_->shared_block_->cmd
#define rtCmdQueue     rtProcess_->shared_block_->commandQueue
#define rtStatusQueue  rtProcess_->shared_block_->statusQueue
#define HeartBeat      rtProcess_->shared_block_->heartBeat
#define AxisCount      rtProcess_->shared_block_->axisCount
#define MultiPlied     rtProcess_->shared_block_->Multiplied
#define ContinueMotion rtProcess_->shared_block_->sacm
#define LastCmdSeq     rtProcess_->shared_block_->lastCmdSeq
#define LastCmdResult  rtProcess_->shared_block_->lastCmdResult


#endif // SHARED_DATA_HPP
#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
 
// ===================================================================
// 1. 定义要在进程间传递的数据结构
// ===================================================================
#define  OUTPUTIOSIZE 32
#define  INPUTIOSIZE  32

constexpr size_t COMMAND_BUFFER_SIZE = 16;
constexpr size_t STATUS_BUFFER_SIZE = 16;
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
};
// 从 NRT -> RT 的命令结构
struct Command {
    char cmd [100];
    // 可以添加更多参数
};

// 从 RT -> NRT 的状态结构
struct Status {
    double current_position{0.0};
    bool is_moving{false};
    uint64_t update_count{0}; // 状态更新计数，方便观察
};

struct systemRegister {
    // 使用数组存储所有布尔寄存器，通过索引访问
    std::array<std::atomic<bool>, OUTPUTIOSIZE> outputIo{}; // 支持32个布尔寄存器
    
    // 保留原有的状态寄存器数组
    std::array<std::atomic<bool>, INPUTIOSIZE> inputIo{}; // 系统状态寄存器
    // std::atomic<std::array<uint64_t, 100>> R;
    // std::atomic<std::array<uint64_t, 100>> C;
    // std::atomic<std::array<uint64_t, 100>> C;
};
struct SharedBlock {
    // NRT -> RT 的命令通道
    SPSCRingBuffer<Command, COMMAND_BUFFER_SIZE> command_queue;

    // RT -> NRT 的状态通道
    SPSCRingBuffer<Status, STATUS_BUFFER_SIZE> status_queue;
    
    systemRegister registers;
    //心跳
    std::atomic<uint64_t>  heartBeat; 
};


#endif // SHARED_DATA_HPP
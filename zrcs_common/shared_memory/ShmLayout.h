#pragma once

// ShmLayout.h — 共享内存布局的唯一真相来源
//
// 包含：
//   - 全局常量（kAxisMax、kShmMagic 等）
//   - ShmHeader（ABI 头部，64 字节，偏移 0）
//   - 数据类型：Command、RtLogEntry、TaskScheduling
//   - ShmSPSC<T,Cap>（仅共享端：head/tail + 槽位数组）
//   - ShmSPSCProducer<T,Cap> / ShmSPSCConsumer<T,Cap>（进程本地包装，不在 SHM）
//   - LockFreeLatest<T>（3-slot seqlock，替代裸数组和状态 SPSC 队列）
//   - lfl_write / lfl_read（LockFreeLatest 的读写函数）
//   - 结果数据类型：JointPosData、FkResultData 等
//   - SharedBlock（整个数据区布局）
//
// 设计原则（LinuxCNC HAL + Orocos RTT 启发）：
//   - 共享内存段 = ShmHeader(64B) | SharedBlock（固定偏移，无堆分配器）
//   - SPSC 缓存变量（cached_tail_ / cached_head_）放在进程本地，绝不入 SHM
//   - 所有 RT↔NRT 共享的可变数组通过 LockFreeLatest 保护，消除数据竞争
//   - 魔数最后写入，作为 RT 初始化完成的 release-acquire 信号

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace zrcs {

// ─────────────────────────────────────────────────────────────────────────────
// 1. 全局常量
// ─────────────────────────────────────────────────────────────────────────────

inline constexpr size_t   kAxisMax       = 64;
inline constexpr size_t   kCmdQueueCap   = 4096; // 必须为 2 的幂（扩容以支持路径 MoveL 批量发送）
inline constexpr size_t   kLogQueueCap   = 256;  // 必须为 2 的幂
inline constexpr size_t   kCmdArgsMax    = 24;
inline constexpr uint32_t kShmMagic      = 0x5A524353u;  // 'ZRCS'
inline constexpr uint32_t kShmVersion    = 10;           // ABI 变更时必须 +1
inline constexpr size_t   kShmTotalSize  = 16 * 1024 * 1024;
inline constexpr const char* kShmName       = "rtMotion";
inline constexpr int         kAttachRetries = 30;
inline constexpr int         kAttachRetryMs = 1000;
inline constexpr const char* kRtProcessName = "zrcsrt";

// 编译期锁自由检查——保证所有跨进程 atomic 均为硬件原语，无 futex 降级
static_assert(std::atomic<uint32_t>::is_always_lock_free,
    "atomic<uint32_t> must be lock-free for cross-process safety");
static_assert(std::atomic<uint64_t>::is_always_lock_free,
    "atomic<uint64_t> must be lock-free for cross-process safety");
static_assert(std::atomic<bool>::is_always_lock_free,
    "atomic<bool> must be lock-free for cross-process safety");
static_assert(std::atomic<double>::is_always_lock_free,
    "atomic<double> must be lock-free for cross-process safety");

// ─────────────────────────────────────────────────────────────────────────────
// 2. ABI 头部（固定在映射起始处，偏移 0，大小 64 字节）
// ─────────────────────────────────────────────────────────────────────────────
//
// RT 初始化 SharedBlock 完毕后，最后一步写入 magic（release 语义）。
// NRT 用 magic.load(acquire) == kShmMagic 判定 RT 已就绪，不再依赖文件存在性轮询。

struct alignas(64) ShmHeader {
    std::atomic<uint32_t> magic{0};  // 最后写入，NRT 侧用 acquire 读
    uint32_t version      = 0;
    uint32_t sizeof_block = 0;
    uint32_t reserved[13] = {};
};
static_assert(sizeof(ShmHeader) == 64, "ShmHeader must be exactly 64 bytes");

// SharedBlock 固定布局偏移
inline constexpr size_t kSharedBlockOffset = sizeof(ShmHeader);  // = 64

// ─────────────────────────────────────────────────────────────────────────────
// 3. 数据类型
// ─────────────────────────────────────────────────────────────────────────────

enum class TaskScheduling : uint32_t {
    RUN,
    ERROR_STATE,
    STOP,
    RESET,
    START,
    SHUTDOWN
};

static_assert(std::atomic<TaskScheduling>::is_always_lock_free,
    "atomic<TaskScheduling> must be lock-free for cross-process safety");

// 命令结构体：用 cmdId 枚举取代 char cmd[100]，消除 RT 热路径中的 strcmp
struct Command {
    double   args[kCmdArgsMax] = {};
    uint32_t seq               = 0;
    uint16_t cmdId             = 0;  // 对应 CmdId 枚举（定义在 CmdArgs.h）
    uint8_t  _pad[2]           = {};
    // sizeof = 20*8 + 4 + 2 + 2 = 172 bytes（旧版 268 bytes）
};

// RT→NRT 日志条目（固定大小，无动态分配）
struct RtLogEntry {
    uint64_t timestamp_us;
    uint8_t  level;       // 0=INFO 1=WARN 2=ERROR
    uint8_t  _pad[1];
    uint16_t line;
    char     file[60];
    char     message[192];
};
static_assert(sizeof(RtLogEntry) == 264, "RtLogEntry layout changed");

struct AxisFeedbackData 
{
    double position[kAxisMax];
    double cmdPosition[kAxisMax];
    double cmdVelocity[kAxisMax];
    double velocity[kAxisMax];
    double torque[kAxisMax];
};



// ─────────────────────────────────────────────────────────────────────────────
// 4. ShmSPSC — 共享端（仅存共享状态，无进程本地缓存）
// ─────────────────────────────────────────────────────────────────────────────
//
// 与旧 SPSCRingBuffer 的关键区别：
//   - 无 cached_tail_ / cached_head_ 字段（这两个缓存属于进程本地，入 SHM 是 bug）
//   - 只暴露 head/tail 原子和 slots 数组
//   - 通过 ShmSPSCProducer / ShmSPSCConsumer 包装器访问

template <typename T, size_t Cap>
struct alignas(64) ShmSPSC {
    static_assert((Cap > 0) && ((Cap & (Cap - 1)) == 0),
        "Capacity must be a power of 2");
    static constexpr size_t kMask = Cap - 1;

    alignas(64) std::atomic<uint32_t> head{0};  // 生产者推进
    alignas(64) std::atomic<uint32_t> tail{0};  // 消费者推进
    T buf[Cap];  // 避免与 Qt `slots` 宏冲突
};

// ─────────────────────────────────────────────────────────────────────────────
// 5. ShmSPSCProducer / ShmSPSCConsumer — 进程本地包装
// ─────────────────────────────────────────────────────────────────────────────
//
// 这两个类不放入共享内存，而是在进程启动时构造，持有对 ShmSPSC 的引用。
// cached_tail_ / cached_head_ 是进程本地优化，即使两进程映射在不同虚拟地址也正确。

template <typename T, size_t Cap>
class ShmSPSCProducer {
public:
    explicit ShmSPSCProducer(ShmSPSC<T, Cap>& shm) noexcept : shm_(shm) {}

    // 无拷贝，生产者是有状态资源
    ShmSPSCProducer(const ShmSPSCProducer&) = delete;
    ShmSPSCProducer& operator=(const ShmSPSCProducer&) = delete;

    // 队列满时静默返回 false（RT 路径不阻塞）
    bool push(const T& item) noexcept {
        const uint32_t head = shm_.head.load(std::memory_order_relaxed);
        const uint32_t next = (head + 1) & ShmSPSC<T, Cap>::kMask;
        if (next == cached_tail_) {
            cached_tail_ = shm_.tail.load(std::memory_order_acquire);
            if (next == cached_tail_) return false;  // 满
        }
        shm_.buf[head] = item;
        shm_.head.store(next, std::memory_order_release);
        return true;
    }

private:
    ShmSPSC<T, Cap>& shm_;
    uint32_t         cached_tail_{0};  // 进程本地，不在 SHM
};

template <typename T, size_t Cap>
class ShmSPSCConsumer {
public:
    explicit ShmSPSCConsumer(ShmSPSC<T, Cap>& shm) noexcept : shm_(shm) {}

    ShmSPSCConsumer(const ShmSPSCConsumer&) = delete;
    ShmSPSCConsumer& operator=(const ShmSPSCConsumer&) = delete;

    // 队列空时返回 false
    bool pop(T& out) noexcept {
        const uint32_t tail = shm_.tail.load(std::memory_order_relaxed);
        if (tail == cached_head_) {
            cached_head_ = shm_.head.load(std::memory_order_acquire);
            if (tail == cached_head_) return false;  // 空
        }
        out = shm_.buf[tail];
        shm_.tail.store((tail + 1) & ShmSPSC<T, Cap>::kMask, std::memory_order_release);
        return true;
    }

private:
    ShmSPSC<T, Cap>& shm_;
    uint32_t         cached_head_{0};  // 进程本地，不在 SHM
};

// ─────────────────────────────────────────────────────────────────────────────
// 6. LockFreeLatest — 3-slot seqlock（Orocos DataObjectLockFree 启发）
// ─────────────────────────────────────────────────────────────────────────────
//
// 用途：替代裸 double[] 数组（数据竞争 UB）和 SPSC 状态队列（NRT 需排空取最新帧）。
// 保证：单写者 + 单读者，NRT 始终读到最新一帧，无数据竞争，无锁，无动态分配。
//
// 协议：
//   写者：version 偶→奇（标记写入中）→ 写 data → 奇→偶+2（完成）→ 更新 latest_idx
//   读者：读 latest_idx → 检查 version 为偶 → 读 data → 重新检查 version 未变 → 成功
// 3 个槽位保证写者不会占用读者正在读的槽位。

template <typename T>
struct alignas(64) LflSlot {
    std::atomic<uint32_t> version{0};  // 偶数=稳定，奇数=写入中
    T data;
};

template <typename T>
struct LockFreeLatest {
    alignas(64) std::atomic<uint32_t> latest_idx{0};
    LflSlot<T> lfl_slots[3];  // 避免与 Qt `slots` 宏冲突
};

// RT 侧写入（单写者，RT-safe：无锁、无分配）
template <typename T>
inline void lfl_write(LockFreeLatest<T>& lfl, const T& val) noexcept {
    const uint32_t cur  = lfl.latest_idx.load(std::memory_order_relaxed);
    const uint32_t next = (cur + 1) % 3;
    auto& slot = lfl.lfl_slots[next];

    const uint32_t v = slot.version.load(std::memory_order_relaxed);
    slot.version.store(v + 1, std::memory_order_relaxed);  // 标为奇数（写入中）
    std::atomic_thread_fence(std::memory_order_release);   // 保证 data 写在 version 奇之后可见

    slot.data = val;

    std::atomic_thread_fence(std::memory_order_release);   // 保证 data 完全写入
    slot.version.store(v + 2, std::memory_order_release);  // 标为偶数（写入完成）
    lfl.latest_idx.store(next, std::memory_order_release); // 发布新索引
}

// NRT 侧读取（单读者，最多重试 4 次；极端情况才失败，调用方下个周期重试即可）
template <typename T>
inline bool lfl_read(const LockFreeLatest<T>& lfl, T& out) noexcept {
    for (int i = 0; i < 4; ++i) {
        const uint32_t idx = lfl.latest_idx.load(std::memory_order_acquire);
        const auto& slot = lfl.lfl_slots[idx];
        const uint32_t v1 = slot.version.load(std::memory_order_acquire);
        if (v1 & 1u) continue;  // 写入中，重试

        out = slot.data;

        std::atomic_thread_fence(std::memory_order_acquire);
        const uint32_t v2 = slot.version.load(std::memory_order_acquire);
        if (v1 == v2) return true;  // 一致读成功
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. LockFreeLatest 使用的结果数据类型
// ─────────────────────────────────────────────────────────────────────────────

struct JointPosData    { double pos[kAxisMax] = {}; };
struct FkResultData    { double pose[6]       = {}; };   // X Y Z RX RY RZ
struct ProbeResultData { double pose[6]       = {}; };
struct CaptureData     { double pos[kAxisMax] = {}; };

// 路径运动路点（NRT 写，RT 读）
struct PathPoint {
    double x  = 0, y  = 0, z  = 0;    // 笛卡尔位置 (mm)
    double rx = 0, ry = 0, rz = 0;    // 姿态 (rad)
    double maxVel = 0;                  // 速度前瞻输出 (mm/s)
};

inline constexpr size_t kPathBufCap = 256;  // 路径缓冲区容量，必须为 2 的幂

// ─────────────────────────────────────────────────────────────────────────────
// 8. SharedBlock — 完整的共享数据布局
// ─────────────────────────────────────────────────────────────────────────────
//
// 位于 kSharedBlockOffset(64) 处，通过 placement-new 构造。
// 所有可变共享数组均通过 LockFreeLatest 保护，消除数据竞争 UB。
// 进程本地 SPSC 包装器（ShmSPSCProducer/Consumer）引用队列字段，不在此结构体中。

struct alignas(64) SharedBlock {

    // ── 命令队列（NRT→RT，单消费者：RT 循环）────────────────────────────
    ShmSPSC<Command,    kCmdQueueCap> cmdQueue;

    // ── 日志队列（RT→NRT，单消费者：NRT 日志线程）──────────────────────
    ShmSPSC<RtLogEntry, kLogQueueCap> logQueue;

    ShmSPSC<AxisFeedbackData, kLogQueueCap> axisFeedbackQueue;  // 额外的队列，用于高频轴状态反馈（可选）

    // ── 任务调度控制（双向，原子读写）────────────────────────────────────
    alignas(64) std::atomic<TaskScheduling> taskSched{TaskScheduling::START};
  

    // ── 系统配置（NRT 写，RT 读）──────────────────────────────────────────
    alignas(64) std::atomic<uint8_t>  axisCount{0};
    alignas(64) std::atomic<double>   overrideRatio{1.0};   // [0.0, 1.0]
    alignas(64) std::atomic<bool>     confJEnabled{true};
    alignas(64) std::atomic<bool>     confLEnabled{true};
    alignas(64) std::atomic<uint8_t>  singAreaMode{0};      // 0=Off 1=Wrist 2=LockAxis

    // ── 连续点动控制（NRT 写，RT 读）─────────────────────────────────────
    struct alignas(64) ContinuousJog {
        std::atomic<int32_t> axisId{0};
        std::atomic<bool>    active{false};
        std::atomic<bool>    direction{true};  // true=正向
    };
    ContinuousJog jogCtrl;

    // ── 命令完成跟踪（RT 写，NRT 读）─────────────────────────────────────
    alignas(64) std::atomic<uint32_t> lastCmdSeq{0};
    alignas(64) std::atomic<uint8_t>  lastCmdResult{0};  // 0=成功，非零=失败码

    // ── IO 读结果（RT 写，NRT 读）────────────────────────────────────────
    alignas(64) std::atomic<uint32_t> ioReadResult{0};

    // ── 轴位置快照（RT 写，NRT 读，LockFreeLatest）──────────────────────
    LockFreeLatest<JointPosData> axisPositions;

    // ── 查询命令结果（RT 写，NRT 读）────────────────────────────────────
    LockFreeLatest<FkResultData>    fkResult;
    LockFreeLatest<JointPosData>    jointPosResult;
    LockFreeLatest<ProbeResultData> probeResult;
    LockFreeLatest<CaptureData>     captureResult;
    LockFreeLatest<uint64_t>        heartbeat;      // 仅用于监测 RT 活跃（每周期递增）
    alignas(64) std::atomic<bool>   probeTriggered{false};
    alignas(64) std::atomic<bool>   captureTriggered{false};

    // ── 路径运动（NRT→RT）──────────────────────────────────────────────
    ShmSPSC<PathPoint, kPathBufCap> pathQueue;              // 路点队列
    alignas(64) std::atomic<bool>   pathMoveActive{false};  // 使能标志

    struct alignas(64) PathMoveConfig {
        std::atomic<double> maxVel{10.0};    // mm/s（与轴配置 motion/maxVel 一致）
        std::atomic<double> maxAccel{20.0};  // mm/s²（与轴配置 motion/maxAcc 一致）
        std::atomic<double> maxJerk{30.0};   // mm/s³（与轴配置 motion/maxJerk 一致）
    };
    PathMoveConfig pathMoveCfg;

    // ── 振镜-平台联动配置（NRT→RT）──────────────────────────────────────
    struct alignas(64) GalvoConfig {
        std::atomic<int32_t> platXId{0};       // 平台 X 轴 ID
        std::atomic<int32_t> platYId{1};       // 平台 Y 轴 ID
        std::atomic<int32_t> galvoXId{2};      // 振镜 X 轴 ID
        std::atomic<int32_t> galvoYId{3};      // 振镜 Y 轴 ID
        std::atomic<double>  cutoffHz{5.0};    // LPF 截止频率 (Hz)
    };
    GalvoConfig galvoCfg;
};

// 内存边界检查
static_assert(kSharedBlockOffset + sizeof(SharedBlock) <= kShmTotalSize,
    "SharedBlock exceeds allocated shared memory region (kShmTotalSize)");

}  // namespace zrcs

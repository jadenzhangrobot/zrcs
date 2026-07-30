#pragma once
/**
 * @file StatusCollector.h
 * @brief 从 RtBridge / BehaviorTreeService 采集状态并写入 StatusStore 的线程。
 *
 * 在状态链路里扮演「拉取者」：RT 侧只负责往共享内存队列里写，不关心有没有人读；
 * 本类起一个后台线程周期性把队列抽空，转交给 StatusStore。
 *
 *   RT 进程 ──写──> 共享内存 SPSC 队列
 *                        │
 *                        └──拉取──> StatusCollector ──写──> StatusStore
 *
 * 采集周期（默认 10ms）与 RT 控制周期（1ms）刻意不同频：GUI 刷新不需要 1ms
 * 粒度，10ms 唤醒一次、每次批量取十帧左右，比 1ms 唤醒一次省得多。代价是
 * 状态最多有一个采集周期的延迟，对显示用途可以接受。
 *
 * 线程安全：本类自身的成员只在构造和 start/stop 时改动；run() 里只读指针成员，
 * 写入目标 StatusStore 内部自带锁。
 */

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#include "shared_memory/ShmLayout.h"

class RtBridge;
class StatusStore;
class BehaviorTreeService;

class StatusCollector {
public:
    /**
     * @param bridge       RT 共享内存桥，反馈帧和心跳的来源，不可为空才有意义
     * @param store        采集结果的写入目标
     * @param behaviorTree 行为树服务，可为空（不需要同步 BT 状态时）
     * @param interval     采集周期，默认 10ms
     *
     * 三个指针都是非拥有的裸指针，生命周期由 NrtApplication 保证长于本对象。
     */
    StatusCollector(RtBridge* bridge,
                    StatusStore* store,
                    BehaviorTreeService* behaviorTree = nullptr,
                    std::chrono::milliseconds interval = std::chrono::milliseconds(10));

    /// 析构时自动 stop()，保证线程先于成员销毁。
    ~StatusCollector();

    // 持有线程和裸指针，禁止拷贝。
    StatusCollector(const StatusCollector&) = delete;
    StatusCollector& operator=(const StatusCollector&) = delete;

    /// 启动采集线程；重复调用是安全的空操作。
    void start();

    /// 置停止标志并 join 线程；重复调用安全。
    void stop();

private:
    /// 采集线程主体。
    void run();

    /// 把任务调度枚举转成 GUI 显示用的字符串。
    static std::string taskStateToString(zrcs::TaskScheduling ts);

    RtBridge* bridge_;                   ///< 非拥有
    StatusStore* store_;                 ///< 非拥有
    BehaviorTreeService* behaviorTree_;  ///< 非拥有，可为 nullptr
    std::chrono::milliseconds interval_; ///< 采集周期
    std::atomic<bool> running_{false};   ///< 线程运行标志，兼作重入保护
    std::thread thread_;
};

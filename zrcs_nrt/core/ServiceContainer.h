#pragma once
/**
 * @file ServiceContainer.h
 * @brief NRT 服务容器 — 集中管理所有服务的创建、依赖注入与生命周期。
 *
 * 设计目标：
 *  - NrtApplication 只持有一个 ServiceContainer，不再逐个管理 10+ 个服务
 *  - 所有服务的构造顺序由本类保证（依赖在前，被依赖在后）
 *  - 裸指针依赖在构造阶段一次性注入，构造完成后依赖关系不可变
 *  - 析构按 unique_ptr 逆序自动完成，无需手动编排
 */

#include <memory>
#include <functional>

#include "shared_memory/ShmLayout.h"

// 前向声明 — 本容器拥有所有服务
class RtBridge;
class CommandService;
class TaskService;
class CommandRouter;
class BehaviorTreeService;
class BehaviorTreeCommandService;
class ZMQServer;
class StatusPublisher;
class RtLogConsumer;

namespace zrcs {
class NrtProcess;
struct RtLogEntry;
}

namespace zrcs_nrt {
class MujocoIdentifyWorker;
struct MujocoIdentifyOptions;
}

/**
 * @brief 服务容器：创建 → 注入 → 初始化 → 启动。
 *
 * 使用方式：
 *   ServiceContainer container(nrtProcess->sharedBlock(), mujocoOptions);
 *   if (!container.initialize()) { ... 失败 ... }
 *   container.startAll();
 *   // ... 主循环 ...
 *   container.shutdown();
 */
class NrtServiceContainer {
public:
    /// @param block  共享内存块（由外部 NrtProcess 提供，生命周期长于本容器）
    /// @param mujocoIdentifyOptions MuJoCo 辨识参数（命令行解析结果）
    NrtServiceContainer(zrcs::SharedBlock* block,
                        const zrcs_nrt::MujocoIdentifyOptions& mujocoIdentifyOptions);

    ~NrtServiceContainer();

    NrtServiceContainer(const NrtServiceContainer&) = delete;
    NrtServiceContainer& operator=(const NrtServiceContainer&) = delete;

    // ---- 初始化与生命周期 ----

    /// 创建 ZMQ socket、绑定端口等可能失败的操作。
    /// 构造阶段只做内存分配，initialize() 做 I/O。
    /// @return true 成功
    bool initialize();

    /// 启动所有后台线程（ZMQ、状态发布、日志消费、辨识）。
    void startAll();

    /// 有序关闭所有后台线程。
    void shutdown();

    // ---- 服务访问器（只读，不转移所有权） ----

    RtBridge*                   bridge() const;
    CommandService*             commandService() const;
    TaskService*                taskService() const;
    CommandRouter*              commandRouter() const;
    BehaviorTreeService*        behaviorTreeService() const;
    BehaviorTreeCommandService* behaviorTreeCommandService() const;
    ZMQServer*                  zmqServer() const;
    StatusPublisher*            statusPublisher() const;
    RtLogConsumer*              rtLogConsumer() const;
    zrcs_nrt::MujocoIdentifyWorker* mujocoIdentifyWorker() const;

private:
    // 共享内存块（外部拥有，本容器只读）
    zrcs::SharedBlock* block_;

    // 服务实例（声明顺序 = 构造顺序 = 析构逆序）
    std::unique_ptr<RtBridge>                   bridge_;
    std::unique_ptr<CommandService>             commandService_;
    std::unique_ptr<TaskService>                taskService_;
    std::unique_ptr<BehaviorTreeService>        behaviorTreeService_;
    std::unique_ptr<BehaviorTreeCommandService> behaviorTreeCommandService_;
    std::unique_ptr<CommandRouter>              commandRouter_;
    std::unique_ptr<ZMQServer>                  zmqServer_;
    std::unique_ptr<StatusPublisher>            statusPublisher_;
    std::unique_ptr<RtLogConsumer>              rtLogConsumer_;
    std::unique_ptr<zrcs_nrt::MujocoIdentifyWorker> mujocoIdentifyWorker_;

    /// 供 RtLogConsumer 回调，将 RT 日志转发到 StatusPublisher
    void onRtLogEntry(const zrcs::RtLogEntry& entry);
};

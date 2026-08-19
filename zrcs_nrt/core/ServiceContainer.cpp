#include "core/ServiceContainer.h"

#include <spdlog/spdlog.h>

#include "algorithm/dynamics_identification/MujocoIdentifyWorker.h"
#include "behavior_tree/core/BehaviorTreeService.h"
#include "command/BehaviorTreeCommandService.h"
#include "command/CommandRouter.h"
#include "command/CommandService.h"
#include "command/TaskService.h"
#include "log/RtLogConsumer.h"
#include "nrtServer/zmq/ZmqServer.h"
#include "rtBridge/RtBridge.h"
#include "status/StatusPublisher.h"

// =============================================================================
// 构造 — 按依赖顺序创建所有服务
// =============================================================================

NrtServiceContainer::NrtServiceContainer(
    zrcs::SharedBlock* block,
    const zrcs_nrt::MujocoIdentifyOptions& mujocoIdentifyOptions)
    : block_(block)
{
    // 第 0 层：共享内存桥（所有服务访问 RT 的唯一入口）
    bridge_ = std::make_unique<RtBridge>(block_);

    // 第 1 层：基础命令服务
    commandService_ = std::make_unique<CommandService>(bridge_.get());
    taskService_    = std::make_unique<TaskService>(bridge_.get(), commandService_.get());

    // 第 2 层：行为树引擎
    behaviorTreeService_ = std::make_unique<BehaviorTreeService>(
        bridge_.get(), commandService_.get(), taskService_.get());
    behaviorTreeCommandService_ =
        std::make_unique<BehaviorTreeCommandService>(behaviorTreeService_.get());

    // 第 3 层：命令路由（聚合三个命令入口）
    commandRouter_ = std::make_unique<CommandRouter>(
        commandService_.get(), taskService_.get(), behaviorTreeCommandService_.get());

    // ZMQ 服务器（REP 端口 5555）
    zmqServer_ = std::make_unique<ZMQServer>(commandRouter_.get());

    // 状态发布（PUB 端口 5556）
    statusPublisher_ = std::make_unique<StatusPublisher>(
        bridge_.get(), behaviorTreeService_.get());

    // MuJoCo 辨识工作线程
    mujocoIdentifyWorker_ = std::make_unique<zrcs_nrt::MujocoIdentifyWorker>(
        block_, mujocoIdentifyOptions);

    // RT 日志消费 → 转发到 StatusPublisher
    rtLogConsumer_ = std::make_unique<RtLogConsumer>(
        block_,
        [this](const zrcs::RtLogEntry& entry) {
            if (statusPublisher_) {
                statusPublisher_->enqueueRtLog(entry);
            }
        });

    spdlog::info("[ServiceContainer] All services constructed");
}

NrtServiceContainer::~NrtServiceContainer()
{
    shutdown();
}

// =============================================================================
// 初始化（I/O 操作，可能失败）
// =============================================================================

bool NrtServiceContainer::initialize()
{
    if (!zmqServer_->initialize()) {
        spdlog::critical("[ServiceContainer] Failed to initialize ZMQ server");
        return false;
    }

    if (!statusPublisher_->initialize()) {
        spdlog::warn("[ServiceContainer] Status publisher failed to initialize, "
                     "continuing without it");
    }

    return true;
}

// =============================================================================
// 启动所有后台线程
// =============================================================================

void NrtServiceContainer::startAll()
{
    mujocoIdentifyWorker_->start();

    zmqServer_->start();
    spdlog::info("[ServiceContainer] ZMQ server started, waiting for commands...");

    if (statusPublisher_->initialize()) {  // 幂等，已初始化则直接返回 true
        statusPublisher_->start();
        spdlog::info("[ServiceContainer] Status publisher started (PUB on port 5556)");
    }

    rtLogConsumer_->start();
    spdlog::info("[ServiceContainer] RT log consumer started");
}

// =============================================================================
// 有序关闭
// =============================================================================

void NrtServiceContainer::shutdown()
{
    // 按照与启动相反的顺序关闭：先停工作线程，再停 ZMQ，最后清理
    if (mujocoIdentifyWorker_) {
        mujocoIdentifyWorker_->stop();
    }
    if (statusPublisher_) {
        statusPublisher_->stop();
    }
    if (rtLogConsumer_) {
        rtLogConsumer_->stop();
    }
    if (behaviorTreeService_) {
        behaviorTreeService_->stop("ServiceContainer shutdown");
    }
    if (zmqServer_) {
        zmqServer_->stop();
    }
}

// =============================================================================
// 访问器
// =============================================================================

RtBridge* NrtServiceContainer::bridge() const { return bridge_.get(); }
CommandService* NrtServiceContainer::commandService() const { return commandService_.get(); }
TaskService* NrtServiceContainer::taskService() const { return taskService_.get(); }
CommandRouter* NrtServiceContainer::commandRouter() const { return commandRouter_.get(); }
BehaviorTreeService* NrtServiceContainer::behaviorTreeService() const { return behaviorTreeService_.get(); }
BehaviorTreeCommandService* NrtServiceContainer::behaviorTreeCommandService() const { return behaviorTreeCommandService_.get(); }
ZMQServer* NrtServiceContainer::zmqServer() const { return zmqServer_.get(); }
StatusPublisher* NrtServiceContainer::statusPublisher() const { return statusPublisher_.get(); }
RtLogConsumer* NrtServiceContainer::rtLogConsumer() const { return rtLogConsumer_.get(); }
zrcs_nrt::MujocoIdentifyWorker* NrtServiceContainer::mujocoIdentifyWorker() const { return mujocoIdentifyWorker_.get(); }

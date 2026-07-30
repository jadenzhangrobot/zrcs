#pragma once
/**
 * @file BehaviorTreeRunner.h
 * @brief 行为树生命周期管理器（tick 线程 + 状态）。
 *
 * 节点注册入口：behavior_tree/core/RegisterNodes。
 */

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <behaviortree_cpp_v3/bt_factory.h>

#include "behavior_tree/core/BtSharedState.h"
#include "behavior_tree/core/BtContext.h"
#include "status/StatusTypes.h"

class RtBridge;

class BehaviorTreeRunner {
public:
    /// 使用完整 BtContext（推荐）。
    explicit BehaviorTreeRunner(const BtContext& context);

    /// 兼容旧构造：仅 bridge（legacy 节点仍可用）。
    explicit BehaviorTreeRunner(RtBridge* bridge);

    ~BehaviorTreeRunner();

    bool loadFromXml(const std::string& xmlText, std::string& error);
    bool start(std::string& error);
    void stop(const std::string& reason = "Stopped");
    zrcs_nrt::BtStatus status() const;

private:
    void registerAllNodes();
    void tickLoop();
    void setStatus(const std::string& treeState,
                   const std::string& currentNode,
                   const std::string& message);

    BtContext context_;
    BT::BehaviorTreeFactory factory_;
    std::unique_ptr<BT::Tree> tree_;
    std::atomic<bool> running_{false};
    std::thread tickThread_;
    mutable std::mutex mutex_;
    mutable std::mutex statusMutex_;
    zrcs_nrt::BtStatus status_;
};

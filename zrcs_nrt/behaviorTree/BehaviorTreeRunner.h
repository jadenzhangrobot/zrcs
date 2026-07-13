#pragma once
/**
 * @file BehaviorTreeRunner.h
 * @brief 行为树生命周期管理器。
 */

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <behaviortree_cpp_v3/bt_factory.h>

#include "behaviorTree/BtSharedState.h"

class RtBridge;

class BehaviorTreeRunner {
public:
    struct StatusSnapshot {
        std::string treeState;     ///< IDLE / LOADED / RUNNING / SUCCESS / FAILURE / HALTED
        std::string currentNode;   ///< 当前正在执行的节点名
        std::string message;       ///< 当前节点的描述文本
    };

    explicit BehaviorTreeRunner(RtBridge* bridge);
    ~BehaviorTreeRunner();

    bool loadFromXml(const std::string& xmlText, std::string& error);
    bool start(std::string& error);
    void stop(const std::string& reason = "Stopped");
    StatusSnapshot status() const;

private:
    void registerNodes();
    void registerCommandAliasNodes();
    template <typename ArgEnum>
    void registerTypedAlias(const std::string& commandName);
    void tickLoop();
    void setStatus(const std::string& treeState,
                   const std::string& currentNode,
                   const std::string& message);

    RtBridge* bridge_;
    BT::BehaviorTreeFactory factory_;
    std::shared_ptr<zrcs_bt::SharedState> sharedState_;
    std::unique_ptr<BT::Tree> tree_;
    std::atomic<bool> running_{false};
    std::thread tickThread_;
    mutable std::mutex mutex_;
    mutable std::mutex statusMutex_;
    StatusSnapshot status_;
};

// 模板实现必须在头文件中可见
#include "behaviorTree/BtCommandNodes.h"

template <typename ArgEnum>
void BehaviorTreeRunner::registerTypedAlias(const std::string& commandName)
{
    BT::TreeNodeManifest manifest{
        BT::NodeType::ACTION,
        commandName,
        zrcs_bt::TypedSendCommandNode<ArgEnum>::providedPorts(),
        "ZRCS RT command"
    };

    factory_.registerBuilder(
        manifest,
        [sharedState = sharedState_, commandName](
            const std::string& name, const BT::NodeConfiguration& config)
        {
            return std::make_unique<zrcs_bt::TypedSendCommandNode<ArgEnum>>(
                name, config, sharedState, commandName);
        });
}

#include "behavior_tree/core/RegisterNodes.h"

#include "behavior_tree/core/BtContext.h"
#include "behavior_tree/core/BtNodeRegistry.h"
#include "behavior_tree/nodes/rtcommand/BtCommandNodes.h"
#include "behavior_tree/nodes/motion/PathMoveNode.h"
#include "config/CmdDefine.h"

namespace {

template <typename ArgEnum>
void registerTypedAlias(BtNodeRegistry& registry,
                        const std::shared_ptr<zrcs_bt::SharedState>& sharedState,
                        const std::string& commandName)
{
    BT::TreeNodeManifest manifest{
        BT::NodeType::ACTION,
        commandName,
        zrcs_bt::TypedSendCommandNode<ArgEnum>::providedPorts(),
        "ZRCS RT command"};

    registry.factory().registerBuilder(
        manifest,
        [sharedState, commandName](const std::string& name, const BT::NodeConfiguration& config) {
            return std::make_unique<zrcs_bt::TypedSendCommandNode<ArgEnum>>(
                name, config, sharedState, commandName);
        });
}

void registerCommandNodes(BtNodeRegistry& registry,
                          const std::shared_ptr<zrcs_bt::SharedState>& sharedState)
{
    registry.factory().registerBuilder<zrcs_bt::SendCommandNode>(
        "SendCommand",
        [sharedState](const std::string& name, const BT::NodeConfiguration& config) {
            return std::make_unique<zrcs_bt::SendCommandNode>(name, config, sharedState);
        });

    registry.factory().registerBuilder<zrcs_bt::BatchCmdNode>(
        "BatchCmd",
        [sharedState](const std::string& name, const BT::NodeConfiguration& config) {
            return std::make_unique<zrcs_bt::BatchCmdNode>(name, config, sharedState);
        });

#define ZRCS_REGISTER_ALIAS(CmdName, ArgType) \
    registerTypedAlias<ArgType>(registry, sharedState, zrcs::cmdIdToName(CmdId::CmdName));
    ZRCS_MOTION_COMMAND_TABLE(ZRCS_REGISTER_ALIAS)
#undef ZRCS_REGISTER_ALIAS
}

void registerMotionNodes(BtNodeRegistry& registry,
                         const std::shared_ptr<zrcs_bt::SharedState>& sharedState)
{
    registry.factory().registerBuilder<zrcs_bt::ButterflyPathNode>(
        "ButterflyPath",
        [sharedState](const std::string& name, const BT::NodeConfiguration& config) {
            return std::make_unique<zrcs_bt::ButterflyPathNode>(name, config, sharedState);
        });

    registry.factory().registerBuilder<zrcs_bt::PathMoveNode>(
        "PathMove",
        [sharedState](const std::string& name, const BT::NodeConfiguration& config) {
            return std::make_unique<zrcs_bt::PathMoveNode>(name, config, sharedState);
        });
}

} // namespace

void registerAllBehaviorTreeNodes(BtNodeRegistry& registry, const BtContext& context)
{
    auto sharedState = context.sharedState;
    if (!sharedState) {
        return;
    }

    registerCommandNodes(registry, sharedState);
    registerMotionNodes(registry, sharedState);
    // 后续视觉 / AI 节点在此追加即可，不必再拆 modules/
}

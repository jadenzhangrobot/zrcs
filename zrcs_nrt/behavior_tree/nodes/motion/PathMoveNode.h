#pragma once
/**
 * @file PathMoveNode.h
 * @brief 连续路径运动节点：waypoints → 拟合/前瞻 → 下发 MovePath。
 *
 * 通常前接 NcParse：
 *   NcParse(waypoints="{pts}") → PathMove(waypoints="{pts}")
 */

#include <memory>
#include <string>

#include <behaviortree_cpp_v3/action_node.h>

#include "behavior_tree/core/BtSharedState.h"

namespace zrcs_bt {

class PathMoveNode : public BT::SyncActionNode {
public:
    PathMoveNode(const std::string& name,
                 const BT::NodeConfiguration& config,
                 std::shared_ptr<SharedState> sharedState);

    static BT::PortsList providedPorts();
    BT::NodeStatus tick() override;

private:
    std::shared_ptr<SharedState> sharedState_;
};

} // namespace zrcs_bt

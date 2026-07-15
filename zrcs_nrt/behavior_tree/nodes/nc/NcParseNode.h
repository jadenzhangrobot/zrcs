#pragma once
/**
 * @file NcParseNode.h
 * @brief NC 程序解析行为树节点（独立于 motion）：.nc → waypoints 黑板。
 *
 * 典型用法：
 *   <Sequence>
 *     <Action ID="NcParse" filePath="config/demo/program/demo_rect.nc"
 *             waypoints="{path_pts}"/>
 *     <Action ID="PathMove" waypoints="{path_pts}" maxVel="100"
 *             cornerTol="0.25" minSegLen="0.05"/>
 *   </Sequence>
 */

#include <memory>
#include <string>

#include <behaviortree_cpp_v3/action_node.h>

#include "behavior_tree/core/BtSharedState.h"

namespace zrcs_bt {

class NcParseNode : public BT::SyncActionNode {
public:
    NcParseNode(const std::string& name,
                const BT::NodeConfiguration& config,
                std::shared_ptr<SharedState> sharedState);

    static BT::PortsList providedPorts();
    BT::NodeStatus tick() override;

private:
    std::shared_ptr<SharedState> sharedState_;
};

} // namespace zrcs_bt

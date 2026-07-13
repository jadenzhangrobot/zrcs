#pragma once
/**
 * @file PathMoveNode.h
 * @brief 运动行为树节点：路径规划 + 下发 MovePath/MoveL。
 *
 * - ButterflyPath：内置示例轨迹（替代旧 MotionPathExecutor 调用）
 * - PathMove：预留通用入口（当前与 Butterfly 相同配置/点集，后续可接端口/文件）
 */

#include <memory>
#include <string>

#include <behaviortree_cpp_v3/action_node.h>

#include "behavior_tree/core/BtSharedState.h"

namespace zrcs_bt {

class ButterflyPathNode : public BT::SyncActionNode {
public:
    ButterflyPathNode(const std::string& name,
                      const BT::NodeConfiguration& config,
                      std::shared_ptr<SharedState> sharedState);

    static BT::PortsList providedPorts();
    BT::NodeStatus tick() override;

private:
    std::shared_ptr<SharedState> sharedState_;
};

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

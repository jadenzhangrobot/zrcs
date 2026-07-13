#include "behavior_tree/nodes/motion/PathMoveNode.h"

#include "algorithm/path_planning/MotionPlanner.h"
#include "algorithm/path_planning/TrajectoryTypes.h"
#include "behavior_tree/nodes/motion/PathMoveSupport.h"

namespace zrcs_bt {
namespace {

MotionPlanner::Config defaultPathConfig()
{
    MotionPlanner::Config cfg;
    cfg.maxVel = 100.0;
    cfg.maxAccel = 300.0;
    cfg.maxJerk = 3000.0;
    cfg.cornerTol = 1.0;
    cfg.galvoMode = false;
    return cfg;
}

std::vector<Point3D> butterflyWaypoints()
{
    return {
        {0.0, 84.0, 0.0},   {-16.0, 68.0, 0.0},  {-48.0, 92.0, 0.0},
        {-90.0, 104.0, 0.0}, {-124.0, 80.0, 0.0}, {-104.0, 40.0, 0.0},
        {-68.0, 14.0, 0.0},  {-110.0, -24.0, 0.0},{-90.0, -76.0, 0.0},
        {-48.0, -62.0, 0.0}, {-16.0, -28.0, 0.0}, {0.0, -72.0, 0.0},
        {16.0, -28.0, 0.0},  {48.0, -62.0, 0.0},  {90.0, -76.0, 0.0},
        {110.0, -24.0, 0.0}, {68.0, 14.0, 0.0},   {104.0, 40.0, 0.0},
        {124.0, 80.0, 0.0},  {90.0, 104.0, 0.0},  {48.0, 92.0, 0.0},
        {16.0, 68.0, 0.0},   {0.0, 84.0, 0.0},
    };
}

} // namespace

ButterflyPathNode::ButterflyPathNode(const std::string& name,
                                     const BT::NodeConfiguration& config,
                                     std::shared_ptr<SharedState> sharedState)
    : BT::SyncActionNode(name, config)
    , sharedState_(std::move(sharedState))
{
}

BT::PortsList ButterflyPathNode::providedPorts()
{
    return {};
}

BT::NodeStatus ButterflyPathNode::tick()
{
    sharedState_->setCurrentNode(name(), "send butterfly path");
    if (!queuePathFromWaypoints(sharedState_->bridge,
                                butterflyWaypoints(),
                                0.1,
                                -0.2,
                                0.3,
                                defaultPathConfig())) {
        sharedState_->setCurrentNode(name(), "failed to queue butterfly path");
        return BT::NodeStatus::FAILURE;
    }
    sharedState_->setCurrentNode(name(), "butterfly path queued");
    return BT::NodeStatus::SUCCESS;
}

PathMoveNode::PathMoveNode(const std::string& name,
                           const BT::NodeConfiguration& config,
                           std::shared_ptr<SharedState> sharedState)
    : BT::SyncActionNode(name, config)
    , sharedState_(std::move(sharedState))
{
}

BT::PortsList PathMoveNode::providedPorts()
{
    // 后续可扩展：filePath / maxVel / cornerTol 等端口
    return {};
}

BT::NodeStatus PathMoveNode::tick()
{
    // 通用路径运动节点：当前默认跑 butterfly 示例；后续接端口/文件。
    sharedState_->setCurrentNode(name(), "send path move");
    if (!queuePathFromWaypoints(sharedState_->bridge,
                                butterflyWaypoints(),
                                0.0,
                                0.0,
                                0.0,
                                defaultPathConfig())) {
        sharedState_->setCurrentNode(name(), "failed to queue path move");
        return BT::NodeStatus::FAILURE;
    }
    sharedState_->setCurrentNode(name(), "path move queued");
    return BT::NodeStatus::SUCCESS;
}

} // namespace zrcs_bt

#include "behavior_tree/nodes/motion/PathMoveNode.h"

#include <vector>

#include <spdlog/spdlog.h>

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
    return cfg;
}

MotionPlanner::Config configFromPorts(BT::TreeNode& node, const MotionPlanner::Config& defaults)
{
    MotionPlanner::Config cfg = defaults;
    if (auto v = node.getInput<double>("maxVel")) {
        cfg.maxVel = *v;
    }
    if (auto v = node.getInput<double>("maxAccel")) {
        cfg.maxAccel = *v;
    }
    if (auto v = node.getInput<double>("maxJerk")) {
        cfg.maxJerk = *v;
    }
    if (auto v = node.getInput<double>("cornerTol")) {
        cfg.cornerTol = *v;
    }
    return cfg;
}

} // namespace

PathMoveNode::PathMoveNode(const std::string& name,
                           const BT::NodeConfiguration& config,
                           std::shared_ptr<SharedState> sharedState)
    : BT::SyncActionNode(name, config)
    , sharedState_(std::move(sharedState))
{
}

BT::PortsList PathMoveNode::providedPorts()
{
    return {
        BT::InputPort<std::vector<Point3D>>("waypoints", "Path points from NcParse (mm)"),
        BT::InputPort<double>("rx", 0.0, "Constant orientation RX (rad)"),
        BT::InputPort<double>("ry", 0.0, "Constant orientation RY (rad)"),
        BT::InputPort<double>("rz", 0.0, "Constant orientation RZ (rad)"),
        BT::InputPort<double>("maxVel", 100.0, "Max path velocity (mm/s)"),
        BT::InputPort<double>("maxAccel", 300.0, "Max path acceleration (mm/s^2)"),
        BT::InputPort<double>("maxJerk", 3000.0, "Max path jerk (mm/s^3)"),
        BT::InputPort<double>("cornerTol", 1.0, "Corner blend tolerance (mm)"),
    };
}

BT::NodeStatus PathMoveNode::tick()
{
    sharedState_->setCurrentNode(name(), "send path move");

    auto waypointsOpt = getInput<std::vector<Point3D>>("waypoints");
    if (!waypointsOpt || waypointsOpt->size() < 2) {
        sharedState_->setCurrentNode(name(), "waypoints missing or < 2 points (use NcParse first)");
        spdlog::error("[PathMove] waypoints port empty or too short");
        return BT::NodeStatus::FAILURE;
    }

    const double rx = getInput<double>("rx").value_or(0.0);
    const double ry = getInput<double>("ry").value_or(0.0);
    const double rz = getInput<double>("rz").value_or(0.0);
    const MotionPlanner::Config cfg = configFromPorts(*this, defaultPathConfig());

    if (!queuePathFromWaypoints(sharedState_->bridge, *waypointsOpt, rx, ry, rz, cfg)) {
        sharedState_->setCurrentNode(name(), "failed to queue path move");
        return BT::NodeStatus::FAILURE;
    }

    const std::string msg = "path move queued (" +
                            std::to_string(waypointsOpt->size()) + " waypoints)";
    sharedState_->setCurrentNode(name(), msg);
    spdlog::info("[PathMove] {}", msg);
    return BT::NodeStatus::SUCCESS;
}

} // namespace zrcs_bt

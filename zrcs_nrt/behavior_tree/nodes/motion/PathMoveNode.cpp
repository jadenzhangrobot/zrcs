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
    // SI 默认：约 100 mm/s、300 mm/s^2、3000 mm/s^3、容差 0.25 mm
    MotionPlanner::Config cfg;
    cfg.maxVel = 0.1;
    cfg.maxAccel = 0.3;
    cfg.maxJerk = 3.0;
    cfg.cornerTol = 0.00025;
    cfg.minSegLen = 0.00005;
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
    if (auto v = node.getInput<double>("minSegLen")) {
        cfg.minSegLen = *v;
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
        BT::InputPort<std::vector<Point3D>>("waypoints", "Path points from NcParse (m)"),
        BT::InputPort<std::vector<double>>("feedrates",
                                           "Optional per-edge feedrates from NcParse (m/s)"),
        BT::InputPort<double>("rx", 0.0, "Start RX / A (rad)"),
        BT::InputPort<double>("ry", 0.0, "Start RY (rad)"),
        BT::InputPort<double>("rz", 0.0, "Start RZ / C (rad)"),
        BT::InputPort<double>("endRx", 0.0, "End RX / A (rad)"),
        BT::InputPort<double>("endRy", 0.0, "End RY (rad)"),
        BT::InputPort<double>("endRz", 0.0, "End RZ / C (rad)"),
        BT::InputPort<double>("maxVel", 0.1, "Max path velocity (m/s)"),
        BT::InputPort<double>("maxAccel", 0.3, "Max path acceleration (m/s^2)"),
        BT::InputPort<double>("maxJerk", 3.0, "Max path jerk (m/s^3)"),
        BT::InputPort<double>("cornerTol", 0.00025, "Corner blend tolerance (m)"),
        BT::InputPort<double>("minSegLen", 0.00005, "Minimum retained line length (m)"),
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
    // 未给 end* 时默认保持起点姿态（常量姿态）
    const double endRx = getInput<double>("endRx").value_or(rx);
    const double endRy = getInput<double>("endRy").value_or(ry);
    const double endRz = getInput<double>("endRz").value_or(rz);
    const MotionPlanner::Config cfg = configFromPorts(*this, defaultPathConfig());

    const auto feedratesOpt = getInput<std::vector<double>>("feedrates");
    const std::vector<double>* feedPtr =
        (feedratesOpt && !feedratesOpt->empty()) ? &(*feedratesOpt) : nullptr;

    if (!queuePathFromWaypoints(sharedState_->bridge, *waypointsOpt,
                                rx, ry, rz, endRx, endRy, endRz, cfg, feedPtr)) {
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

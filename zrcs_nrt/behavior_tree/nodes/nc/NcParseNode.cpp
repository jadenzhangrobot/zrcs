#include "behavior_tree/nodes/nc/NcParseNode.h"

#include <vector>

#include <spdlog/spdlog.h>

#include "core/nc_parser/NcParser.h"
#include "algorithm/path_planning/TrajectoryTypes.h"
#include "config/ProjectConfig.h"

namespace zrcs_bt {

NcParseNode::NcParseNode(const std::string& name,
                         const BT::NodeConfiguration& config,
                         std::shared_ptr<SharedState> sharedState)
    : BT::SyncActionNode(name, config)
    , sharedState_(std::move(sharedState))
{
}

BT::PortsList NcParseNode::providedPorts()
{
    return {
        BT::InputPort<std::string>("filePath", "Path to .nc / G-code file"),
        BT::InputPort<double>("arcChordTol", 0.0002, "Arc sampling chord height (m)"),
        BT::OutputPort<std::vector<Point3D>>("waypoints", "Parsed path points (m)"),
        BT::OutputPort<std::vector<double>>("feedrates",
                                            "Per-edge feedrates (m/s); <=0 falls back to PathMove maxVel"),
    };
}

BT::NodeStatus NcParseNode::tick()
{
    auto filePath = getInput<std::string>("filePath");
    if (!filePath || filePath->empty()) {
        sharedState_->setCurrentNode(name(), "missing filePath");
        spdlog::error("[NcParse] filePath port is required");
        return BT::NodeStatus::FAILURE;
    }

    // XML 里写 config/<proj>/program/*.nc（相对工程根）；NRT 常在 build/bin 下运行
    const std::string resolved = zrcs::ProjectConfig::resolvePath(*filePath);

    NcParser::Config cfg;
    if (auto tol = getInput<double>("arcChordTol")) {
        cfg.arcChordTol = *tol;
    }

    sharedState_->setCurrentNode(name(), "parse " + resolved);

    NcParser parser(cfg);
    const auto result = parser.parseFile(resolved);
    if (!result.error.empty()) {
        sharedState_->setCurrentNode(name(), result.error);
        spdlog::error("[NcParse] {} (input='{}')", result.error, *filePath);
        return BT::NodeStatus::FAILURE;
    }

    setOutput("waypoints", result.waypoints);
    setOutput("feedrates", result.segmentFeedrates);
    const std::string msg = "parsed " + std::to_string(result.waypoints.size()) +
                            " waypoints / " +
                            std::to_string(result.segmentFeedrates.size()) +
                            " feed edges from " + std::to_string(result.moveCount) +
                            " moves (" + resolved + ")";
    sharedState_->setCurrentNode(name(), msg);
    spdlog::info("[NcParse] {}", msg);
    return BT::NodeStatus::SUCCESS;
}

} // namespace zrcs_bt

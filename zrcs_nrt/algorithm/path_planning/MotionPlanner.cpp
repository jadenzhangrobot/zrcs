#include "algorithm/path_planning/MotionPlanner.h"

#include <cmath>

bool MotionPlanner::plan(const std::vector<Point3D>& waypoints,
                              double rx,
                              double ry,
                              double rz,
                              const Config& cfg,
                              std::vector<TrajectorySegment>& segments,
                              std::string* error)
{
    segments.clear();

    if (waypoints.size() < 2) {
        if (error) {
            *error = "Need at least 2 waypoints";
        }
        return false;
    }

    auto blocks = makeBlocksFromWaypoints(waypoints, rx, ry, rz, cfg.maxVel);
    segments = pathFitter_.fitCornerBlendSegments(blocks, cfg.cornerTol);
    if (segments.empty()) {
        if (error) {
            *error = "Path fitting produced no trajectory segments";
        }
        return false;
    }

    velPlanner_.setConfig(cfg.maxVel, cfg.maxAccel, cfg.startVel, cfg.endVel, cfg.cornerTol, cfg.maxJerk);
    if (!velPlanner_.planSegments(segments)) {
        if (error) {
            *error = "Segment velocity planning failed";
        }
        segments.clear();
        return false;
    }

    return true;
}

std::vector<PathMoveBlock> MotionPlanner::makeBlocksFromWaypoints(
    const std::vector<Point3D>& waypoints,
    double rx,
    double ry,
    double rz,
    double feedrate)
{
    std::vector<PathMoveBlock> blocks;
    blocks.reserve(waypoints.size() - 1);
    for (size_t i = 1; i < waypoints.size(); ++i) {
        PathMoveBlock block;
        block.block_id = static_cast<int>(i - 1);
        block.type = PathMoveType::Line;
        block.start = waypoints[i - 1];
        block.end = waypoints[i];
        block.rx = rx;
        block.ry = ry;
        block.rz = rz;
        block.feedrate = feedrate;
        blocks.push_back(block);
    }
    return blocks;
}

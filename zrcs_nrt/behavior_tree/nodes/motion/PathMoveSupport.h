#pragma once
/**
 * @file PathMoveSupport.h
 * @brief 路径规划 + 打包下发（供运动行为树节点与测试复用）。
 *
 * 规划：algorithm/path_planning/MotionPlanner
 * 发令：RtBridge（不并入 CommandService）
 */

#include "algorithm/path_planning/MotionPlanner.h"
#include "algorithm/path_planning/TrajectoryTypes.h"

#include <vector>

class RtBridge;

namespace zrcs_bt {

/// 规划 waypoints 并下发 MovePath / MoveL / MoveLGalvo。
bool queuePathFromWaypoints(RtBridge* bridge,
                            const std::vector<Point3D>& waypoints,
                            double rx,
                            double ry,
                            double rz,
                            const MotionPlanner::Config& cfg);

/// 仅下发已规划段。
bool queuePlannedSegments(RtBridge* bridge,
                          const std::vector<TrajectorySegment>& segments,
                          double rx,
                          double ry,
                          double rz,
                          const MotionPlanner::Config& cfg);

} // namespace zrcs_bt

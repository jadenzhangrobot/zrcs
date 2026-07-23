#pragma once
/**
 * @file PathMoveSupport.h
 * @brief 路径规划 + 打包下发 MovePath（连续路径唯一出口）。
 *
 * 规划：algorithm/path_planning/MotionPlanner
 * 发令：RtBridge → RT 命令 "MovePath"
 */

#include "algorithm/path_planning/MotionPlanner.h"
#include "algorithm/path_planning/TrajectoryTypes.h"

#include <vector>

class RtBridge;

namespace zrcs_bt {

/// 规划 waypoints 并下发 MovePath。
/// 姿态可沿路径弧长从首姿态插到末姿态，也可由 waypointOrientations 逐点指定。
/// @param segmentFeedrates 可选段进给 (m/s)，与 waypoints 相邻边对齐；nullptr 时用 cfg.maxVel。
/// @param waypointOrientations 可选逐点 A/B/C 姿态；使用时几何规划不得改变点边对应关系。
bool queuePathFromWaypoints(RtBridge* bridge,
                            const std::vector<Point3D>& waypoints,
                            double rx,
                            double ry,
                            double rz,
                            double endRx,
                            double endRy,
                            double endRz,
                            const MotionPlanner::Config& cfg,
                            const std::vector<double>* segmentFeedrates = nullptr,
                            const std::vector<PathOrientation>* waypointOrientations = nullptr);

/// 仅下发已规划段为 MovePath。
bool queuePlannedSegments(RtBridge* bridge,
                          const std::vector<TrajectorySegment>& segments,
                          double rx,
                          double ry,
                          double rz,
                          double endRx,
                          double endRy,
                          double endRz,
                          const MotionPlanner::Config& cfg,
                          const std::vector<PathOrientation>* waypointOrientations = nullptr);

} // namespace zrcs_bt

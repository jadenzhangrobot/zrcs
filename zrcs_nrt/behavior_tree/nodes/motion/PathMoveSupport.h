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
/// 姿态沿路径弧长从 (rx,ry,rz) 插到 (endRx,endRy,endRz)。
/// @param segmentFeedrates 可选段进给 (m/s)，与 waypoints 相邻边对齐；nullptr 时用 cfg.maxVel。
bool queuePathFromWaypoints(RtBridge* bridge,
                            const std::vector<Point3D>& waypoints,
                            double rx,
                            double ry,
                            double rz,
                            double endRx,
                            double endRy,
                            double endRz,
                            const MotionPlanner::Config& cfg,
                            const std::vector<double>* segmentFeedrates = nullptr);

/// 仅下发已规划段为 MovePath。
bool queuePlannedSegments(RtBridge* bridge,
                          const std::vector<TrajectorySegment>& segments,
                          double rx,
                          double ry,
                          double rz,
                          double endRx,
                          double endRy,
                          double endRz,
                          const MotionPlanner::Config& cfg);

} // namespace zrcs_bt

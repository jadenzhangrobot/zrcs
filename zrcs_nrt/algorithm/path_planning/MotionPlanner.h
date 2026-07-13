#pragma once
/**
 * @file MotionPlanner.h
 * @brief 纯路径预处理：waypoints → 几何拟合 + 速度前瞻段序列。
 *
 * 不依赖 RtBridge / ZMQ / SHM。发令见 behavior_tree/nodes/motion/PathMoveSupport（运动行为树节点）。
 */

#include "algorithm/path_planning/PathPreprocessor.h"
#include "algorithm/path_planning/TrajectoryTypes.h"
#include "algorithm/path_planning/VelocityPlanner3D.h"

#include <string>
#include <vector>

class MotionPlanner {
public:
    struct Config {
        double maxVel = 10.0;
        double maxAccel = 20.0;
        double maxJerk = 30.0;
        double cornerTol = 0.5;
        double startVel = 0.0;
        double endVel = 0.0;
        bool galvoMode = false; ///< 仅语义标记；纯规划不读此字段发令
    };

    MotionPlanner() = default;

    /// 规划成功返回 true，segments 为拟合+前瞻结果；失败时 error 有说明。
    bool plan(const std::vector<Point3D>& waypoints,
              double rx,
              double ry,
              double rz,
              const Config& cfg,
              std::vector<TrajectorySegment>& segments,
              std::string* error = nullptr);

private:
    static std::vector<PathMoveBlock> makeBlocksFromWaypoints(const std::vector<Point3D>& waypoints,
                                                              double rx,
                                                              double ry,
                                                              double rz,
                                                              double feedrate);

    PathPreprocessor pathFitter_;
    VelocityPlanner3D velPlanner_;
};

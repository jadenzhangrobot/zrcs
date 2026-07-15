#pragma once

/**
 * @file MotionPlanner.h
 * @brief 路径规划编排入口：减点 → 拐角 → 短段合并 → 速度前瞻。
 *
 * 职责边界：
 * - 本类只做纯算法编排，不依赖 RtBridge / ZMQ / SHM
 * - 行为树发令见 behavior_tree/nodes/motion/PathMoveSupport
 * - G 代码解析见 core/nc_parser/NcParser
 *
 * 流水线：
 *   waypoints / PathMoveBlock
 *     → ① PathSimplifier::collapseCollinearWaypoints
 *     → ② CornerBlender::fitCornerBlendSegments
 *     → ③ ShortSegmentMerger::merge
 *     → ④ LookAheadPlanner::planSegments
 *   → TrajectorySegment[]（几何 + 速度）
 */

#include "algorithm/path_planning/LookAheadPlanner.h"
#include "algorithm/path_planning/TrajectoryTypes.h"

#include <string>
#include <vector>

class MotionPlanner {
public:
    /// 完整规划配置。
    struct Config {
        double maxVel = 10.0;         ///< 全局最大速度 (mm/s)
        double maxAccel = 20.0;       ///< 最大加速度
        double maxJerk = 30.0;        ///< 最大加加速度
        double cornerTol = 0.25;      ///< 拐角路径容差 (mm)，兼作默认共线容差
        double startVel = 0.0;        ///< 路径起点速度
        double endVel = 0.0;          ///< 路径终点速度
        double collinearTol = -1.0;   ///< 共线折叠弦高；<=0 时默认 = cornerTol
        double minSegLen = 0.05;      ///< 最短直线段；minChordForBlend = 4*minSegLen
    };

    MotionPlanner() = default;

    /// 几何阶段（①–③）：blocks → 简化 + 拐角 + 短段合并，不做速度前瞻。
    /// 供测试或只需要几何段的调用方使用。
    static std::vector<TrajectorySegment> buildGeometry(
        const std::vector<PathMoveBlock>& blocks,
        double cornerTol,
        double collinearTol = -1.0,
        double minSegLen = 0.05);

    /// 完整规划：几何 + 速度前瞻。
    /// @return true 成功；false 时 error 有说明，segments 清空
    bool plan(const std::vector<Point3D>& waypoints,
              double rx,
              double ry,
              double rz,
              const Config& cfg,
              std::vector<TrajectorySegment>& segments,
              std::string* error = nullptr);

private:
    /// waypoints → 相邻点构成的直线 PathMoveBlock 序列。
    static std::vector<PathMoveBlock> makeBlocksFromWaypoints(const std::vector<Point3D>& waypoints,
                                                              double rx,
                                                              double ry,
                                                              double rz,
                                                              double feedrate);

    LookAheadPlanner lookAhead_;
};

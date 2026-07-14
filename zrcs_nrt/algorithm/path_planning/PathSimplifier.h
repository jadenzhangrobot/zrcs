#pragma once

/// @file PathSimplifier.h
/// @brief 路径简化（流水线第 ① 步，对应 LinuxCNC G64 Q / naive-CAM）。
///
/// 两类操作：
/// 1) collapseCollinearWaypoints：在点列上折叠近共线中间点（减点）
/// 2) mergeShortLineSegments：在段序列上合并过短/近共线直线（圆角后清理也会调用）

#include "algorithm/path_planning/TrajectoryTypes.h"

#include <vector>

class PathSimplifier {
public:
    /// 折叠近共线中间点，减少 CAM 密化带来的微段。
    ///
    /// @param points     N 个路径点（就地改写）
    /// @param feedrates  N-1 段进给（与 points 同步改写）
    /// @param collinearTol 弦高容差；<=0 时直接返回
    /// @param sourceIds  可选，N-1 段源 block id；非空时与 feedrates 同步折叠
    ///
    /// 规则：中间点到 [anchor, 候选终点] 的垂距均 ≤ collinearTol，
    /// 且相邻进给足够接近时，才丢弃中间点。
    static void collapseCollinearWaypoints(std::vector<Point3D>& points,
                                           std::vector<double>& feedrates,
                                           double collinearTol,
                                           std::vector<int>* sourceIds = nullptr);

    /// 合并连续的过短/近共线直线段，避免 OTG 在微段上超调回退。
    ///
    /// @param minSegLen    短于此长度且方向接近时允许强制合并
    /// @param collinearTol 弦高容差；近共线合并时使用
    static void mergeShortLineSegments(std::vector<TrajectorySegment>& segments,
                                       double minSegLen,
                                       double collinearTol);
};

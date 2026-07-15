#pragma once

/// @file LookAheadPlanner.h
/// @brief 速度前瞻（流水线第 ④ 步）。
///
/// 在已定几何段序列上规划速度：
/// 1) 段内速度上限（进给、曲率-向心加速度）
/// 2) 接点速度上限（拐角容差、曲率跳变/jerk）
/// 3) 反向扫描：保证能减速到后续接点
/// 4) 正向扫描：保证能从入口加速到后续接点
/// 5) 对固定接点速度执行 (v,a) 联合规划
/// 6) 写入 v_enter / v_exit / a_enter / a_exit / duration
///
/// 不依赖 RT 头文件；控制周期用 ZRCS_CYCLE_TIME_MS（默认 10ms）。

#include "algorithm/path_planning/TrajectoryTypes.h"

#include <vector>

class LookAheadPlanner {
public:
    LookAheadPlanner();

    /// @param maxV      全局最大速度
    /// @param maxA      最大加速度（同时用于向心限速）
    /// @param startV    路径起点速度
    /// @param endV      路径终点速度
    /// @param cornerTol 接点路径容差（影响拐角限速）
    /// @param maxJ      最大加加速度（jerk）
    void setConfig(double maxV,
                   double maxA,
                   double startV = 0.0,
                   double endV = 0.0,
                   double cornerTol = 0.5,
                   double maxJ = 1000.0);

    /// 就地写入速度、边界加速度、持续时间及优化标志。
    bool planSegments(std::vector<TrajectorySegment>& segments);

private:
    double maxVelGlobal_ = 100.0;
    double maxAccel_ = 100.0;
    double maxJerk_ = 1000.0;
    double startVel_ = 0.0;
    double endVel_ = 0.0;
    double cornerTolerance_ = 0.5;

    /// 段内速度上限：min(全局, 进给, sqrt(a/κ))
    double calculateLocalVelocityLimit(const TrajectorySegment& segment) const;

    /// 接点速度上限：拐角容差模型 + 曲率跳变 jerk 限制
    double calculateJunctionVelocityLimit(const TrajectorySegment& prev,
                                          const TrajectorySegment& next) const;

    /// 在 jerk/加速度约束下，速度从 v0 变到 v1 所需时间
    double transitionTime(double v0, double v1) const;

    /// 完成该速度变化所需最短距离
    double dMinTransition(double v0, double v1) const;

    /// 从 v0 出发、在 distance 内最多能达到的速度
    double maxReachableVel(double v0, double distance) const;

    /// 在固定接点速度下，用 Ruckig 可行转移做离散动态规划，联合选择接点加速度。
    bool planJunctionAccelerations(const std::vector<double>& junctionVel,
                                   std::vector<TrajectorySegment>& segments) const;

    /// 返回给定边界状态的最短转移时间；不可行时返回正无穷。
    double transitionDuration(double distance,
                              double velocityLimit,
                              double v0,
                              double a0,
                              double v1,
                              double a1) const;
};

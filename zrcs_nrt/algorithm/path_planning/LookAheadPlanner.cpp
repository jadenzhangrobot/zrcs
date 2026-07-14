#include "algorithm/path_planning/LookAheadPlanner.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// 算法层不依赖 RT 头文件；周期来自 CMake 注入的 ZRCS_CYCLE_TIME_MS（ms）。
#ifndef ZRCS_CYCLE_TIME_MS
#define ZRCS_CYCLE_TIME_MS 10
#endif

LookAheadPlanner::LookAheadPlanner() = default;

void LookAheadPlanner::setConfig(double maxV,
                                 double maxA,
                                 double startV,
                                 double endV,
                                 double cornerTol,
                                 double maxJ)
{
    maxVelGlobal_ = maxV;
    maxAccel_ = maxA;
    maxJerk_ = maxJ;
    startVel_ = startV;
    endVel_ = endV;
    cornerTolerance_ = cornerTol;
}

bool LookAheadPlanner::planSegments(std::vector<TrajectorySegment>& segments)
{
    if (segments.empty()) {
        std::cerr << "Error: trajectory must contain at least one segment." << std::endl;
        return false;
    }

    // ---- 1) 段内速度上限 ----
    for (auto& segment : segments) {
        if (segment.length <= 1e-9 || !std::isfinite(segment.length)) {
            std::cerr << "Error: invalid trajectory segment length." << std::endl;
            return false;
        }
        segment.v_max_local = calculateLocalVelocityLimit(segment);
    }

    // junctionVel[i]：第 i 个接点速度；共 N+1 个接点（含起终点）
    std::vector<double> junctionVel(segments.size() + 1, maxVelGlobal_);
    junctionVel.front() = std::clamp(startVel_, 0.0, maxVelGlobal_);
    junctionVel.back() = std::clamp(endVel_, 0.0, maxVelGlobal_);

    // ---- 2) 几何接点限速 ----
    for (size_t i = 1; i < junctionVel.size() - 1; ++i) {
        double limit = std::min(segments[i - 1].v_max_local, segments[i].v_max_local);
        limit = std::min(limit, calculateJunctionVelocityLimit(segments[i - 1], segments[i]));
        junctionVel[i] = std::clamp(limit, 0.0, maxVelGlobal_);
    }

    // ---- 3) 反向扫描：保证能减速到后续接点 ----
    // 从终点往前：若入口太高，在本段长度内刹不住，则压低入口
    for (int i = static_cast<int>(segments.size()) - 1; i >= 0; --i) {
        const double reachable = maxReachableVel(junctionVel[i + 1], segments[i].length);
        junctionVel[i] = std::min(junctionVel[i], reachable);
    }

    // ---- 4) 正向扫描：保证能从入口加速到后续接点 ----
    for (size_t i = 0; i < segments.size(); ++i) {
        const double reachable = maxReachableVel(junctionVel[i], segments[i].length);
        junctionVel[i + 1] = std::min(junctionVel[i + 1], reachable);
    }

    // ---- 5) 写回每段 ----
    for (size_t i = 0; i < segments.size(); ++i) {
        auto& segment = segments[i];
        segment.v_enter = std::clamp(junctionVel[i], 0.0, segment.v_max_local);
        segment.v_exit = std::clamp(junctionVel[i + 1], 0.0, segment.v_max_local);
        segment.duration = estimateSegmentDuration(segment);
        segment.is_lookahead_optimized = true;
    }

    return true;
}

double LookAheadPlanner::calculateLocalVelocityLimit(const TrajectorySegment& segment) const
{
    double limit = maxVelGlobal_;
    if (segment.feedrate_limit > 0.0) {
        limit = std::min(limit, segment.feedrate_limit);
    }
    // 向心加速度约束：a_n = v^2 * κ  ≤ maxAccel  →  v ≤ sqrt(a/κ)
    if (segment.max_curvature > 1e-9) {
        limit = std::min(limit, std::sqrt(maxAccel_ / segment.max_curvature));
    }
    return std::max(0.0, limit);
}

double LookAheadPlanner::calculateJunctionVelocityLimit(const TrajectorySegment& prev,
                                                        const TrajectorySegment& next) const
{
    // 拐角模型：用切向夹角与路径容差估计允许过弯速度
    // v^2 ≈ a * cornerTol / (1 - cosθ)
    const Point3D tPrev = pointNormalize(segmentTangent(prev, 1.0));
    const Point3D tNext = pointNormalize(segmentTangent(next, 0.0));
    const double cosTheta = std::clamp(pointDot(tPrev, tNext), -1.0, 1.0);
    const double denom = std::max(1.0 - cosTheta, 1e-6);
    const double vCorner = std::sqrt(maxAccel_ * cornerTolerance_ / denom);
    // 反向/大折角时 directionFactor→0，强制更慢
    const double directionFactor = std::max(0.0, cosTheta);
    double limit = vCorner * directionFactor;

    // 曲率跳变引起的法向加速度变化受 jerk 限制
    const double kPrev = segmentCurvature(prev, 1.0);
    const double kNext = segmentCurvature(next, 0.0);
    const double deltaK = std::abs(kNext - kPrev);
    if (deltaK > 1e-9) {
        constexpr double kCurvatureRampTime = 0.02; // s，曲率过渡时间下界
        const double jerkWindow =
            std::max(kCurvatureRampTime, static_cast<double>(ZRCS_CYCLE_TIME_MS) * 0.001);
        const double accelJumpLimit = std::max(maxJerk_ * jerkWindow, 1e-9);
        // Δa ≈ v^2 * Δκ  ≤ accelJumpLimit
        const double vCurvatureJump = std::sqrt(accelJumpLimit / deltaK);
        limit = std::min(limit, vCurvatureJump);
    }

    return std::min(maxVelGlobal_, limit);
}

double LookAheadPlanner::transitionTime(double v0, double v1) const
{
    // S 曲线速度切换时间（简化）：
    // 若 jerk 限制的峰值加速度未触顶：T = 2*sqrt(dv/j)
    // 否则：梯形加速度剖面 T = 2*(a/j) + (dv/a - a/j)
    const double dv = std::abs(v1 - v0);
    if (dv < 1e-9) {
        return 0.0;
    }
    const double aPeak = std::sqrt(maxJerk_ * dv);
    if (aPeak <= maxAccel_) {
        return 2.0 * std::sqrt(dv / maxJerk_);
    }

    const double tj = maxAccel_ / maxJerk_;
    const double ta = dv / maxAccel_ - tj;
    return 2.0 * tj + ta;
}

double LookAheadPlanner::dMinTransition(double v0, double v1) const
{
    // 平均速度 × 时间
    return 0.5 * (v0 + v1) * transitionTime(v0, v1);
}

double LookAheadPlanner::maxReachableVel(double v0, double distance) const
{
    // 二分：在 distance 内从 v0 最多能到多高
    double lo = v0;
    double hi = maxVelGlobal_;
    if (dMinTransition(v0, hi) <= distance) {
        return hi;
    }

    for (int iter = 0; iter < 64; ++iter) {
        const double mid = 0.5 * (lo + hi);
        if (dMinTransition(v0, mid) <= distance) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return lo;
}

double LookAheadPlanner::estimateSegmentDuration(const TrajectorySegment& segment) const
{
    const double vmax = std::max({segment.v_enter, segment.v_exit, segment.v_max_local});
    if (vmax <= 1e-9) {
        return 0.0;
    }

    // 若能加速到 v_max_local 再减速：梯形速度
    const double dAccel = dMinTransition(segment.v_enter, segment.v_max_local);
    const double dDecel = dMinTransition(segment.v_exit, segment.v_max_local);
    if (dAccel + dDecel <= segment.length) {
        const double cruise = segment.length - dAccel - dDecel;
        return transitionTime(segment.v_enter, segment.v_max_local) +
               transitionTime(segment.v_exit, segment.v_max_local) +
               cruise / segment.v_max_local;
    }

    // 否则三角形：找段内峰值速度 lo，使加减速距离刚好吃满段长
    double lo = std::max(segment.v_enter, segment.v_exit);
    double hi = segment.v_max_local;
    for (int iter = 0; iter < 64; ++iter) {
        const double mid = 0.5 * (lo + hi);
        const double needed = dMinTransition(segment.v_enter, mid) +
                              dMinTransition(segment.v_exit, mid);
        if (needed <= segment.length) {
            lo = mid;
        } else {
            hi = mid;
        }
    }

    return transitionTime(segment.v_enter, lo) +
           transitionTime(segment.v_exit, lo);
}

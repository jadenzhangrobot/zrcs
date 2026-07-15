#include "algorithm/path_planning/LookAheadPlanner.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

#include <ruckig/ruckig.hpp>

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
    if (!std::isfinite(maxVelGlobal_) || maxVelGlobal_ <= 0.0 ||
        !std::isfinite(maxAccel_) || maxAccel_ <= 0.0 ||
        !std::isfinite(maxJerk_) || maxJerk_ <= 0.0 ||
        !std::isfinite(startVel_) || !std::isfinite(endVel_) ||
        !std::isfinite(cornerTolerance_) || cornerTolerance_ < 0.0) {
        std::cerr << "Error: invalid velocity planning configuration." << std::endl;
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
    junctionVel.front() = std::clamp(startVel_, 0.0, segments.front().v_max_local);
    junctionVel.back() = std::clamp(endVel_, 0.0, segments.back().v_max_local);

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

    // ---- 5) 先写回接点速度 ----
    for (size_t i = 0; i < segments.size(); ++i) {
        auto& segment = segments[i];
        segment.v_enter = std::clamp(junctionVel[i], 0.0, segment.v_max_local);
        segment.v_exit = std::clamp(junctionVel[i + 1], 0.0, segment.v_max_local);
    }

    // ---- 6) 在固定接点速度下联合选择接点加速度 ----
    if (!planJunctionAccelerations(junctionVel, segments)) {
        std::cerr << "Error: no jerk-limited acceleration plan for trajectory." << std::endl;
        return false;
    }

    for (auto& segment : segments) {
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

double LookAheadPlanner::transitionDuration(double distance,
                                            double velocityLimit,
                                            double v0,
                                            double a0,
                                            double v1,
                                            double a1) const
{
    if (distance <= 1e-9 || velocityLimit <= 0.0) {
        return std::numeric_limits<double>::infinity();
    }

    ruckig::Ruckig<1> otg;
    ruckig::InputParameter<1> input;
    ruckig::Trajectory<1> trajectory;

    input.current_position = {0.0};
    input.current_velocity = {v0};
    input.current_acceleration = {a0};
    input.target_position = {distance};
    input.target_velocity = {v1};
    input.target_acceleration = {a1};
    input.max_velocity = {velocityLimit};
    input.min_velocity = std::array<double, 1>{0.0};
    input.max_acceleration = {maxAccel_};
    input.max_jerk = {maxJerk_};

    const auto result = otg.calculate(input, trajectory);
    if (result < ruckig::Result::Working ||
        !std::isfinite(trajectory.get_duration()) ||
        trajectory.get_duration() <= 0.0) {
        return std::numeric_limits<double>::infinity();
    }

    return trajectory.get_duration();
}

bool LookAheadPlanner::planJunctionAccelerations(
    const std::vector<double>& junctionVel,
    std::vector<TrajectorySegment>& segments) const
{
    constexpr int kHalfUniformSamples = 8;
    constexpr double kStateEpsilon = 1e-9;
    const double infinity = std::numeric_limits<double>::infinity();
    const size_t junctionCount = junctionVel.size();

    std::vector<std::vector<double>> accelerationCandidates(junctionCount);
    for (size_t i = 0; i < junctionCount; ++i) {
        auto& candidates = accelerationCandidates[i];
        if (i == 0 || i + 1 == junctionCount || junctionVel[i] <= kStateEpsilon) {
            candidates.push_back(0.0);
            continue;
        }

        candidates.reserve(2 * kHalfUniformSamples + 4);
        for (int sample = -kHalfUniformSamples; sample <= kHalfUniformSamples; ++sample) {
            candidates.push_back(maxAccel_ * static_cast<double>(sample) /
                                 static_cast<double>(kHalfUniformSamples));
        }

        const double leftEquivalent =
            (junctionVel[i] * junctionVel[i] - junctionVel[i - 1] * junctionVel[i - 1]) /
            (2.0 * segments[i - 1].length);
        const double rightEquivalent =
            (junctionVel[i + 1] * junctionVel[i + 1] - junctionVel[i] * junctionVel[i]) /
            (2.0 * segments[i].length);
        const double centralEstimate =
            (junctionVel[i + 1] * junctionVel[i + 1] -
             junctionVel[i - 1] * junctionVel[i - 1]) /
            (2.0 * (segments[i - 1].length + segments[i].length));

        candidates.push_back(std::clamp(leftEquivalent, -maxAccel_, maxAccel_));
        candidates.push_back(std::clamp(rightEquivalent, -maxAccel_, maxAccel_));
        candidates.push_back(std::clamp(centralEstimate, -maxAccel_, maxAccel_));

        std::sort(candidates.begin(), candidates.end());
        candidates.erase(
            std::unique(candidates.begin(), candidates.end(), [](double lhs, double rhs) {
                return std::abs(lhs - rhs) <= 1e-10;
            }),
            candidates.end());
    }

    std::vector<std::vector<double>> costToEnd(junctionCount);
    std::vector<std::vector<int>> nextChoice(segments.size());
    std::vector<std::vector<double>> chosenDuration(segments.size());

    costToEnd.back().assign(accelerationCandidates.back().size(), infinity);
    costToEnd.back()[0] = 0.0;

    for (size_t reverse = segments.size(); reverse > 0; --reverse) {
        const size_t i = reverse - 1;
        const auto& currentCandidates = accelerationCandidates[i];
        const auto& nextCandidates = accelerationCandidates[i + 1];

        costToEnd[i].assign(currentCandidates.size(), infinity);
        nextChoice[i].assign(currentCandidates.size(), -1);
        chosenDuration[i].assign(currentCandidates.size(), infinity);

        for (size_t currentIndex = 0; currentIndex < currentCandidates.size(); ++currentIndex) {
            for (size_t nextIndex = 0; nextIndex < nextCandidates.size(); ++nextIndex) {
                if (!std::isfinite(costToEnd[i + 1][nextIndex])) {
                    continue;
                }

                const double duration = transitionDuration(
                    segments[i].length,
                    segments[i].v_max_local,
                    junctionVel[i],
                    currentCandidates[currentIndex],
                    junctionVel[i + 1],
                    nextCandidates[nextIndex]);
                if (!std::isfinite(duration)) {
                    continue;
                }

                const double candidateCost = duration + costToEnd[i + 1][nextIndex];
                if (candidateCost + 1e-12 < costToEnd[i][currentIndex]) {
                    costToEnd[i][currentIndex] = candidateCost;
                    nextChoice[i][currentIndex] = static_cast<int>(nextIndex);
                    chosenDuration[i][currentIndex] = duration;
                }
            }
        }
    }

    if (costToEnd.empty() || costToEnd.front().empty() ||
        !std::isfinite(costToEnd.front()[0])) {
        return false;
    }

    size_t currentIndex = 0;
    for (size_t i = 0; i < segments.size(); ++i) {
        const int selectedNext = nextChoice[i][currentIndex];
        if (selectedNext < 0) {
            return false;
        }

        segments[i].a_enter = accelerationCandidates[i][currentIndex];
        segments[i].a_exit = accelerationCandidates[i + 1][static_cast<size_t>(selectedNext)];
        segments[i].duration = chosenDuration[i][currentIndex];
        currentIndex = static_cast<size_t>(selectedNext);
    }

    return true;
}

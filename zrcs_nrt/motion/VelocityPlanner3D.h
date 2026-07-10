#pragma once

#include "TrajectoryTypes.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

class VelocityPlanner3D {
public:
    VelocityPlanner3D()
        : max_vel_global_(100.0),
          max_accel_(100.0),
          max_jerk_(1000.0),
          start_vel_(0.0),
          end_vel_(0.0),
          corner_tolerance_(0.5)
    {
    }

    void setConfig(double max_v,
                   double max_a,
                   double start_v = 0.0,
                   double end_v = 0.0,
                   double corner_tol = 0.5,
                   double max_j = 1000.0)
    {
        max_vel_global_ = max_v;
        max_accel_ = max_a;
        max_jerk_ = max_j;
        start_vel_ = start_v;
        end_vel_ = end_v;
        corner_tolerance_ = corner_tol;
    }

    bool planSegments(std::vector<TrajectorySegment>& segments)
    {
        if (segments.empty()) {
            std::cerr << "Error: trajectory must contain at least one segment." << std::endl;
            return false;
        }

        for (auto& segment : segments) {
            if (segment.length <= 1e-9 || !std::isfinite(segment.length)) {
                std::cerr << "Error: invalid trajectory segment length." << std::endl;
                return false;
            }
            segment.v_max_local = calculateLocalVelocityLimit(segment);
            segment.a_max_local = max_accel_;
            segment.jerk_max_local = max_jerk_;
        }

        std::vector<double> junctionVel(segments.size() + 1, max_vel_global_);
        junctionVel.front() = std::clamp(start_vel_, 0.0, max_vel_global_);
        junctionVel.back() = std::clamp(end_vel_, 0.0, max_vel_global_);

        for (size_t i = 1; i < junctionVel.size() - 1; ++i) {
            double limit = std::min(segments[i - 1].v_max_local, segments[i].v_max_local);
            limit = std::min(limit, calculateJunctionVelocityLimit(segments[i - 1], segments[i]));
            junctionVel[i] = std::clamp(limit, 0.0, max_vel_global_);
        }

        for (int i = static_cast<int>(segments.size()) - 1; i >= 0; --i) {
            const double reachable = maxReachableVel(junctionVel[i + 1], segments[i].length);
            junctionVel[i] = std::min(junctionVel[i], reachable);
        }

        for (size_t i = 0; i < segments.size(); ++i) {
            const double reachable = maxReachableVel(junctionVel[i], segments[i].length);
            junctionVel[i + 1] = std::min(junctionVel[i + 1], reachable);
        }

        for (size_t i = 0; i < segments.size(); ++i) {
            auto& segment = segments[i];
            segment.v_enter = std::clamp(junctionVel[i], 0.0, segment.v_max_local);
            segment.v_exit = std::clamp(junctionVel[i + 1], 0.0, segment.v_max_local);
            segment.duration = estimateSegmentDuration(segment);
            segment.is_lookahead_optimized = true;
        }

        return true;
    }

private:
    double max_vel_global_;
    double max_accel_;
    double max_jerk_;
    double start_vel_;
    double end_vel_;
    double corner_tolerance_;

    double calculateLocalVelocityLimit(const TrajectorySegment& segment) const
    {
        double limit = max_vel_global_;
        if (segment.feedrate_limit > 0.0) {
            limit = std::min(limit, segment.feedrate_limit);
        }
        if (segment.max_curvature > 1e-9) {
            limit = std::min(limit, std::sqrt(max_accel_ / segment.max_curvature));
        }
        return std::max(0.0, limit);
    }

    double calculateJunctionVelocityLimit(const TrajectorySegment& prev,
                                          const TrajectorySegment& next) const
    {
        const Point3D tPrev = pointNormalize(segmentTangent(prev, 1.0));
        const Point3D tNext = pointNormalize(segmentTangent(next, 0.0));
        const double cosTheta = std::clamp(pointDot(tPrev, tNext), -1.0, 1.0);
        const double denom = std::max(1.0 - cosTheta, 1e-6);
        const double vCorner = std::sqrt(max_accel_ * corner_tolerance_ / denom);
        const double directionFactor = std::max(0.0, cosTheta);
        double limit = vCorner * directionFactor;

        const double kPrev = segmentCurvature(prev, 1.0);
        const double kNext = segmentCurvature(next, 0.0);
        const double deltaK = std::abs(kNext - kPrev);
        if (deltaK > 1e-9) {
            constexpr double kCurvatureRampTime = 0.02; // seconds
            const double jerkWindow = std::max(kCurvatureRampTime, cycletime * 0.001);
            const double accelJumpLimit = std::max(max_jerk_ * jerkWindow, 1e-9);
            const double vCurvatureJump = std::sqrt(accelJumpLimit / deltaK);
            limit = std::min(limit, vCurvatureJump);
        }

        return std::min(max_vel_global_, limit);
    }

    double transitionTime(double v0, double v1) const
    {
        const double dv = std::abs(v1 - v0);
        if (dv < 1e-9) {
            return 0.0;
        }
        const double aPeak = std::sqrt(max_jerk_ * dv);
        if (aPeak <= max_accel_) {
            return 2.0 * std::sqrt(dv / max_jerk_);
        }

        const double tj = max_accel_ / max_jerk_;
        const double ta = dv / max_accel_ - tj;
        return 2.0 * tj + ta;
    }

    double dMinTransition(double v0, double v1) const
    {
        return 0.5 * (v0 + v1) * transitionTime(v0, v1);
    }

    double maxReachableVel(double v0, double distance) const
    {
        double lo = v0;
        double hi = max_vel_global_;
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

    double estimateSegmentDuration(const TrajectorySegment& segment) const
    {
        const double vmax = std::max({segment.v_enter, segment.v_exit, segment.v_max_local});
        if (vmax <= 1e-9) {
            return 0.0;
        }

        const double dAccel = dMinTransition(segment.v_enter, segment.v_max_local);
        const double dDecel = dMinTransition(segment.v_exit, segment.v_max_local);
        if (dAccel + dDecel <= segment.length) {
            const double cruise = segment.length - dAccel - dDecel;
            return transitionTime(segment.v_enter, segment.v_max_local) +
                   transitionTime(segment.v_exit, segment.v_max_local) +
                   cruise / segment.v_max_local;
        }

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
};

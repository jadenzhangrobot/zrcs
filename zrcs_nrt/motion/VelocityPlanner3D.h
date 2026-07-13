#pragma once

#include "motion/TrajectoryTypes.h"

#include <vector>

class VelocityPlanner3D {
public:
    VelocityPlanner3D();

    void setConfig(double max_v,
                   double max_a,
                   double start_v = 0.0,
                   double end_v = 0.0,
                   double corner_tol = 0.5,
                   double max_j = 1000.0);

    bool planSegments(std::vector<TrajectorySegment>& segments);

private:
    double max_vel_global_;
    double max_accel_;
    double max_jerk_;
    double start_vel_;
    double end_vel_;
    double corner_tolerance_;

    double calculateLocalVelocityLimit(const TrajectorySegment& segment) const;
    double calculateJunctionVelocityLimit(const TrajectorySegment& prev,
                                          const TrajectorySegment& next) const;
    double transitionTime(double v0, double v1) const;
    double dMinTransition(double v0, double v1) const;
    double maxReachableVel(double v0, double distance) const;
    double estimateSegmentDuration(const TrajectorySegment& segment) const;
};

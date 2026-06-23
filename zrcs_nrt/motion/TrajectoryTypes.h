#pragma once

#include <algorithm>
#include <cmath>

struct Point3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

enum class PathMoveType {
    Line
};

struct PathMoveBlock {
    int block_id = 0;
    PathMoveType type = PathMoveType::Line;

    Point3D start;
    Point3D end;

    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;

    // Programmed feedrate in mm/s. Values <= 0 fall back to the global max velocity.
    double feedrate = 0.0;
};

enum class TrajectorySegmentType {
    Line,
    CubicPolynomial,
    CircularArc
};

struct TrajectorySegment {
    int segment_id = 0;
    int source_block_id = 0;

    TrajectorySegmentType type = TrajectorySegmentType::Line;

    double length = 0.0;
    double coeff[6][4] = {}; // axis, c0 + c1*u + c2*u^2 + c3*u^3, u in [0, 1]
    Point3D arc_center;
    Point3D arc_u;
    Point3D arc_v;
    double arc_radius = 0.0;
    double arc_sweep = 0.0;
    double max_curvature = 0.0;

    double feedrate_limit = 0.0;
    double v_max_local = 0.0;

    double v_enter = 0.0;
    double v_exit = 0.0;
    double a_max_local = 0.0;
    double jerk_max_local = 0.0;
    double duration = 0.0;

    bool is_lookahead_optimized = false;
};

inline Point3D pointAdd(const Point3D& a, const Point3D& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Point3D pointSub(const Point3D& a, const Point3D& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Point3D pointScale(const Point3D& p, double s)
{
    return {p.x * s, p.y * s, p.z * s};
}

inline double pointDot(const Point3D& a, const Point3D& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Point3D pointCross(const Point3D& a, const Point3D& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

inline double pointLength(const Point3D& p)
{
    return std::sqrt(pointDot(p, p));
}

inline double pointDistance(const Point3D& a, const Point3D& b)
{
    return pointLength(pointSub(a, b));
}

inline Point3D pointNormalize(const Point3D& p)
{
    const double len = pointLength(p);
    if (len < 1e-12) {
        return {};
    }
    return pointScale(p, 1.0 / len);
}

inline double evalSegmentAxis(const TrajectorySegment& seg, int axis, double u)
{
    u = std::clamp(u, 0.0, 1.0);
    const double u2 = u * u;
    const double u3 = u2 * u;
    return seg.coeff[axis][0] +
           seg.coeff[axis][1] * u +
           seg.coeff[axis][2] * u2 +
           seg.coeff[axis][3] * u3;
}

inline double evalSegmentAxisDerivative(const TrajectorySegment& seg, int axis, double u)
{
    u = std::clamp(u, 0.0, 1.0);
    return seg.coeff[axis][1] +
           2.0 * seg.coeff[axis][2] * u +
           3.0 * seg.coeff[axis][3] * u * u;
}

inline double evalSegmentAxisSecondDerivative(const TrajectorySegment& seg, int axis, double u)
{
    u = std::clamp(u, 0.0, 1.0);
    return 2.0 * seg.coeff[axis][2] +
           6.0 * seg.coeff[axis][3] * u;
}

inline Point3D evaluateSegment(const TrajectorySegment& seg, double u)
{
    if (seg.type == TrajectorySegmentType::CircularArc) {
        u = std::clamp(u, 0.0, 1.0);
        const double theta = seg.arc_sweep * u;
        const double c = std::cos(theta);
        const double s = std::sin(theta);
        return pointAdd(seg.arc_center,
                        pointScale(pointAdd(pointScale(seg.arc_u, c),
                                            pointScale(seg.arc_v, s)),
                                   seg.arc_radius));
    }

    return {
        evalSegmentAxis(seg, 0, u),
        evalSegmentAxis(seg, 1, u),
        evalSegmentAxis(seg, 2, u),
    };
}

inline Point3D segmentTangent(const TrajectorySegment& seg, double u)
{
    if (seg.type == TrajectorySegmentType::CircularArc) {
        u = std::clamp(u, 0.0, 1.0);
        const double theta = seg.arc_sweep * u;
        const double c = std::cos(theta);
        const double s = std::sin(theta);
        return pointScale(pointAdd(pointScale(seg.arc_u, -s),
                                   pointScale(seg.arc_v, c)),
                          seg.arc_radius * seg.arc_sweep);
    }

    return {
        evalSegmentAxisDerivative(seg, 0, u),
        evalSegmentAxisDerivative(seg, 1, u),
        evalSegmentAxisDerivative(seg, 2, u),
    };
}

inline Point3D segmentSecondDerivative(const TrajectorySegment& seg, double u)
{
    if (seg.type == TrajectorySegmentType::CircularArc) {
        u = std::clamp(u, 0.0, 1.0);
        const double theta = seg.arc_sweep * u;
        const double c = std::cos(theta);
        const double s = std::sin(theta);
        return pointScale(pointAdd(pointScale(seg.arc_u, -c),
                                   pointScale(seg.arc_v, -s)),
                          seg.arc_radius * seg.arc_sweep * seg.arc_sweep);
    }

    return {
        evalSegmentAxisSecondDerivative(seg, 0, u),
        evalSegmentAxisSecondDerivative(seg, 1, u),
        evalSegmentAxisSecondDerivative(seg, 2, u),
    };
}

inline double segmentCurvature(const TrajectorySegment& seg, double u)
{
    const Point3D d1 = segmentTangent(seg, u);
    const Point3D d2 = segmentSecondDerivative(seg, u);
    const double speed = pointLength(d1);
    if (speed < 1e-9) {
        return 0.0;
    }

    return pointLength(pointCross(d1, d2)) / (speed * speed * speed);
}

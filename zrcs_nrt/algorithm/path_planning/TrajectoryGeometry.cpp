#include "algorithm/path_planning/TrajectoryGeometry.h"

#include <algorithm>
#include <cmath>

double TrajectoryGeometry::pointLineDistance(const Point3D& p, const Point3D& a, const Point3D& b)
{
    // |AP × AB| / |AB| ：点到直线距离
    const Point3D ab = pointSub(b, a);
    const double abLen2 = pointDot(ab, ab);
    if (abLen2 < 1e-24) {
        // A、B 重合，退化为点距
        return pointDistance(p, a);
    }
    const Point3D ap = pointSub(p, a);
    const Point3D cross = pointCross(ap, ab);
    return pointLength(cross) / std::sqrt(abLen2);
}

double TrajectoryGeometry::mergeFeedrate(double a, double b)
{
    // 工艺上合并段取更保守（更慢）的进给
    if (a > 0.0 && b > 0.0) {
        return std::min(a, b);
    }
    return a > 0.0 ? a : b;
}

void TrajectoryGeometry::setAxis(TrajectorySegment& segment, int axis, double c0, double c1, double c2, double c3)
{
    segment.coeff[axis][0] = c0;
    segment.coeff[axis][1] = c1;
    segment.coeff[axis][2] = c2;
    segment.coeff[axis][3] = c3;
}

void TrajectoryGeometry::finalizeGeometry(TrajectorySegment& segment)
{
    segment.length = estimateLength(segment);
    segment.max_curvature = estimateMaxCurvature(segment);
}

double TrajectoryGeometry::estimateLength(const TrajectorySegment& segment)
{
    if (segment.type == TrajectorySegmentType::CircularArc) {
        return std::abs(segment.arc_radius * segment.arc_sweep);
    }
    // 直线：端点欧氏距离（与 c1 系数一致）
    return pointDistance(evaluateSegment(segment, 0.0), evaluateSegment(segment, 1.0));
}

double TrajectoryGeometry::estimateMaxCurvature(const TrajectorySegment& segment)
{
    if (segment.type == TrajectorySegmentType::Line) {
        return 0.0;
    }
    if (segment.type == TrajectorySegmentType::CircularArc) {
        return segment.arc_radius > 1e-9 ? 1.0 / segment.arc_radius : 0.0;
    }
    return 0.0;
}

TrajectorySegment TrajectoryGeometry::makeLineSegment(int segmentId,
                                                      int sourceBlockId,
                                                      const Point3D& start,
                                                      const Point3D& end,
                                                      double rx,
                                                      double ry,
                                                      double rz,
                                                      double feedrate)
{
    TrajectorySegment segment;
    segment.segment_id = segmentId;
    segment.source_block_id = sourceBlockId;
    segment.type = TrajectorySegmentType::Line;
    segment.feedrate_limit = feedrate;

    // 位置：p(u) = start + (end-start)*u
    setAxis(segment, 0, start.x, end.x - start.x, 0.0, 0.0);
    setAxis(segment, 1, start.y, end.y - start.y, 0.0, 0.0);
    setAxis(segment, 2, start.z, end.z - start.z, 0.0, 0.0);
    // 姿态：当前按整段常数（无插值）
    setAxis(segment, 3, rx, 0.0, 0.0, 0.0);
    setAxis(segment, 4, ry, 0.0, 0.0, 0.0);
    setAxis(segment, 5, rz, 0.0, 0.0, 0.0);

    finalizeGeometry(segment);
    return segment;
}

TrajectorySegment TrajectoryGeometry::makeArcSegment(int segmentId,
                                                     int sourceBlockId,
                                                     const CornerBlendGeom& corner,
                                                     double rx,
                                                     double ry,
                                                     double rz,
                                                     double feedrate)
{
    TrajectorySegment segment;
    segment.segment_id = segmentId;
    segment.source_block_id = sourceBlockId;
    segment.type = TrajectorySegmentType::CircularArc;
    segment.feedrate_limit = feedrate;
    segment.arc_center = corner.center;
    segment.arc_u = corner.basisU;
    segment.arc_v = corner.basisV;
    segment.arc_radius = corner.radius;
    segment.arc_sweep = corner.sweep;
    // 姿态仍写常数轴，便于下游统一读 coeff[3..5]
    setAxis(segment, 3, rx, 0.0, 0.0, 0.0);
    setAxis(segment, 4, ry, 0.0, 0.0, 0.0);
    setAxis(segment, 5, rz, 0.0, 0.0, 0.0);

    finalizeGeometry(segment);
    return segment;
}

#pragma once

#include "algorithm/path_planning/TrajectoryTypes.h"

#include <array>
#include <vector>

class PathPreprocessor {
public:
    std::vector<TrajectorySegment> fitCornerBlendSegments(
        const std::vector<PathMoveBlock>& blocks, double cornerTol);

    static Point3D evaluate(const TrajectorySegment& segment, double u);
    static Point3D tangent(const TrajectorySegment& segment, double u);
    static double curvature(const TrajectorySegment& segment, double u);

private:
    struct CornerBlend {
        bool active = false;
        Point3D start;
        Point3D end;
        Point3D center;
        Point3D basisU;
        Point3D basisV;
        double radius = 0.0;
        double sweep = 0.0;

        const Point3D& startPoint() const { return start; }
        const Point3D& endPoint() const { return end; }
    };

    static void buildCornerBlends(const std::vector<Point3D>& raw,
                                  double cornerTol,
                                  std::vector<CornerBlend>& corners);
    static TrajectorySegment makeLineSegment(int segmentId,
                                             int sourceBlockId,
                                             const Point3D& start,
                                             const Point3D& end,
                                             double rx,
                                             double ry,
                                             double rz,
                                             double feedrate);
    static TrajectorySegment makeArcSegment(int segmentId,
                                            int sourceBlockId,
                                            const CornerBlend& corner,
                                            double rx,
                                            double ry,
                                            double rz,
                                            double feedrate);
    static TrajectorySegment makeCubicSpanSegment(int segmentId,
                                                  int sourceBlockId,
                                                  const std::array<Point3D, 5>& ctrl,
                                                  double u0,
                                                  double u1,
                                                  double rx,
                                                  double ry,
                                                  double rz,
                                                  double feedrate);
    static void setAxis(TrajectorySegment& segment,
                        int axis,
                        double c0,
                        double c1,
                        double c2,
                        double c3);
    static std::array<double, 4> fitCubicFromSamples(double y0, double y1, double y2, double y3);
    static void finalizeGeometry(TrajectorySegment& segment);
    static double estimateLength(const TrajectorySegment& segment);
    static double integrateSegmentSpeed(const TrajectorySegment& segment, double u);
    static double estimateMaxCurvature(const TrajectorySegment& segment);
    static double mergeFeedrate(double a, double b);
    static Point3D evalClampedCubicBSpline(const std::array<Point3D, 5>& ctrl, double u);
};

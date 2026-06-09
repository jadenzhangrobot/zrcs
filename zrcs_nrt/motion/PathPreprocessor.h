#pragma once

#include "TrajectoryTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

class PathPreprocessor {
public:
    std::vector<TrajectorySegment> fitCornerBlendSegments(
        const std::vector<PathMoveBlock>& blocks, double sampleStep, double cornerTol)
    {
        std::vector<PathMoveBlock> linearBlocks;
        linearBlocks.reserve(blocks.size());
        for (const auto& block : blocks) {
            if (block.type == PathMoveType::Line &&
                pointDistance(block.start, block.end) > 1e-9) {
                linearBlocks.push_back(block);
            }
        }

        if (linearBlocks.empty()) {
            return {};
        }

        const double safeStep = std::max(sampleStep, 1e-6);
        std::vector<Point3D> raw;
        raw.reserve(linearBlocks.size() + 1);
        raw.push_back(linearBlocks.front().start);
        for (const auto& block : linearBlocks) {
            raw.push_back(block.end);
        }

        const size_t N = raw.size();
        std::vector<CornerBlend> corners(N);
        if (N > 2) {
            buildCornerBlends(raw, safeStep, cornerTol, corners);
        }

        std::vector<TrajectorySegment> segments;
        int segmentId = 0;

        for (size_t i = 0; i < linearBlocks.size(); ++i) {
            const auto& block = linearBlocks[i];
            const Point3D segStart = (i > 0 && corners[i].active)
                                   ? corners[i].end()
                                   : raw[i];
            const Point3D segEnd = (i + 1 < N - 1 && corners[i + 1].active)
                                 ? corners[i + 1].start()
                                 : raw[i + 1];

            if (pointDistance(segStart, segEnd) > 1e-9) {
                segments.push_back(makeLineSegment(
                    segmentId++, block.block_id, segStart, segEnd,
                    block.rx, block.ry, block.rz, block.feedrate, safeStep));
            }

            if (i + 1 < N - 1 && corners[i + 1].active) {
                const double cornerFeedrate = mergeFeedrate(
                    linearBlocks[i].feedrate, linearBlocks[i + 1].feedrate);
                segments.push_back(makeCubicSpanSegment(
                    segmentId++, block.block_id, corners[i + 1].ctrl, 0.0, 0.5,
                    block.rx, block.ry, block.rz, cornerFeedrate, safeStep));
                segments.push_back(makeCubicSpanSegment(
                    segmentId++, block.block_id, corners[i + 1].ctrl, 0.5, 1.0,
                    block.rx, block.ry, block.rz, cornerFeedrate, safeStep));
            }
        }

        return segments;
    }

    static Point3D evaluate(const TrajectorySegment& segment, double u)
    {
        return evaluateSegment(segment, u);
    }

    static Point3D tangent(const TrajectorySegment& segment, double u)
    {
        return segmentTangent(segment, u);
    }

    static double curvature(const TrajectorySegment& segment, double u)
    {
        return segmentCurvature(segment, u);
    }

private:
    struct CornerBlend {
        bool active = false;
        std::array<Point3D, 5> ctrl{};

        const Point3D& start() const { return ctrl.front(); }
        const Point3D& end() const { return ctrl.back(); }
    };

    static void buildCornerBlends(const std::vector<Point3D>& raw,
                                  double sampleStep,
                                  double cornerTol,
                                  std::vector<CornerBlend>& corners)
    {
        for (size_t i = 1; i + 1 < raw.size(); ++i) {
            const Point3D vIn = pointNormalize(pointSub(raw[i], raw[i - 1]));
            const Point3D vOut = pointNormalize(pointSub(raw[i + 1], raw[i]));
            const double cosTheta = std::clamp(pointDot(vIn, vOut), -1.0, 1.0);
            const double theta = std::acos(cosTheta);

            if (theta < 0.01) {
                continue;
            }

            const double alpha = theta * 0.5;
            const double cosA = std::cos(alpha);
            const double dTol = (1.0 - cosA) > 1e-9
                              ? cornerTol * cosA / (1.0 - cosA)
                              : 1e6;
            const double lIn = pointDistance(raw[i], raw[i - 1]);
            const double lOut = pointDistance(raw[i + 1], raw[i]);
            const double dMax = 0.5 * std::min(lIn, lOut);
            const double d = std::min(dTol, dMax);

            if (d < sampleStep) {
                continue;
            }

            constexpr double k = 0.5;
            const Point3D start = pointSub(raw[i], pointScale(vIn, d));
            const Point3D end = pointAdd(raw[i], pointScale(vOut, d));
            corners[i].active = true;
            corners[i].ctrl = {
                start,
                pointAdd(start, pointScale(vIn, k * d)),
                raw[i],
                pointSub(end, pointScale(vOut, k * d)),
                end,
            };
        }
    }

    static TrajectorySegment makeLineSegment(int segmentId,
                                             int sourceBlockId,
                                             const Point3D& start,
                                             const Point3D& end,
                                             double rx,
                                             double ry,
                                             double rz,
                                             double feedrate,
                                             double sampleStep)
    {
        TrajectorySegment segment;
        segment.segment_id = segmentId;
        segment.source_block_id = sourceBlockId;
        segment.type = TrajectorySegmentType::Line;
        segment.feedrate_limit = feedrate;

        setAxis(segment, 0, start.x, end.x - start.x, 0.0, 0.0);
        setAxis(segment, 1, start.y, end.y - start.y, 0.0, 0.0);
        setAxis(segment, 2, start.z, end.z - start.z, 0.0, 0.0);
        setAxis(segment, 3, rx, 0.0, 0.0, 0.0);
        setAxis(segment, 4, ry, 0.0, 0.0, 0.0);
        setAxis(segment, 5, rz, 0.0, 0.0, 0.0);

        finalizeGeometry(segment, sampleStep);
        return segment;
    }

    static TrajectorySegment makeCubicSpanSegment(int segmentId,
                                                  int sourceBlockId,
                                                  const std::array<Point3D, 5>& ctrl,
                                                  double u0,
                                                  double u1,
                                                  double rx,
                                                  double ry,
                                                  double rz,
                                                  double feedrate,
                                                  double sampleStep)
    {
        const Point3D p0 = evalClampedCubicBSpline(ctrl, u0);
        const Point3D p1 = evalClampedCubicBSpline(ctrl, u0 + (u1 - u0) / 3.0);
        const Point3D p2 = evalClampedCubicBSpline(ctrl, u0 + 2.0 * (u1 - u0) / 3.0);
        const Point3D p3 = evalClampedCubicBSpline(ctrl, u1);

        TrajectorySegment segment;
        segment.segment_id = segmentId;
        segment.source_block_id = sourceBlockId;
        segment.type = TrajectorySegmentType::CubicPolynomial;
        segment.feedrate_limit = feedrate;

        const auto cx = fitCubicFromSamples(p0.x, p1.x, p2.x, p3.x);
        const auto cy = fitCubicFromSamples(p0.y, p1.y, p2.y, p3.y);
        const auto cz = fitCubicFromSamples(p0.z, p1.z, p2.z, p3.z);
        setAxis(segment, 0, cx[0], cx[1], cx[2], cx[3]);
        setAxis(segment, 1, cy[0], cy[1], cy[2], cy[3]);
        setAxis(segment, 2, cz[0], cz[1], cz[2], cz[3]);
        setAxis(segment, 3, rx, 0.0, 0.0, 0.0);
        setAxis(segment, 4, ry, 0.0, 0.0, 0.0);
        setAxis(segment, 5, rz, 0.0, 0.0, 0.0);

        finalizeGeometry(segment, sampleStep);
        return segment;
    }

    static void setAxis(TrajectorySegment& segment,
                        int axis,
                        double c0,
                        double c1,
                        double c2,
                        double c3)
    {
        segment.coeff[axis][0] = c0;
        segment.coeff[axis][1] = c1;
        segment.coeff[axis][2] = c2;
        segment.coeff[axis][3] = c3;
    }

    static std::array<double, 4> fitCubicFromSamples(double y0, double y1, double y2, double y3)
    {
        double m[3][4] = {
            {1.0 / 3.0, 1.0 / 9.0, 1.0 / 27.0, y1 - y0},
            {2.0 / 3.0, 4.0 / 9.0, 8.0 / 27.0, y2 - y0},
            {1.0,       1.0,       1.0,        y3 - y0},
        };

        for (int col = 0; col < 3; ++col) {
            int pivot = col;
            for (int row = col + 1; row < 3; ++row) {
                if (std::abs(m[row][col]) > std::abs(m[pivot][col])) {
                    pivot = row;
                }
            }
            if (pivot != col) {
                for (int k = col; k < 4; ++k) {
                    std::swap(m[col][k], m[pivot][k]);
                }
            }

            const double denom = std::abs(m[col][col]) > 1e-12 ? m[col][col] : 1.0;
            for (int k = col; k < 4; ++k) {
                m[col][k] /= denom;
            }
            for (int row = 0; row < 3; ++row) {
                if (row == col) {
                    continue;
                }
                const double factor = m[row][col];
                for (int k = col; k < 4; ++k) {
                    m[row][k] -= factor * m[col][k];
                }
            }
        }

        return {y0, m[0][3], m[1][3], m[2][3]};
    }

    static void finalizeGeometry(TrajectorySegment& segment, double sampleStep)
    {
        segment.length = estimateLength(segment, sampleStep);
        segment.max_curvature = estimateMaxCurvature(segment);
    }

    static double estimateLength(const TrajectorySegment& segment, double sampleStep)
    {
        if (segment.type == TrajectorySegmentType::Line) {
            return pointDistance(evaluateSegment(segment, 0.0), evaluateSegment(segment, 1.0));
        }

        const Point3D a = evaluateSegment(segment, 0.0);
        const Point3D b = evaluateSegment(segment, 1.0);
        const int sampleCount = std::max(16, static_cast<int>(std::ceil(
            std::max(pointDistance(a, b), sampleStep) / sampleStep)));

        double length = 0.0;
        Point3D prev = a;
        for (int i = 1; i <= sampleCount; ++i) {
            const double u = static_cast<double>(i) / sampleCount;
            const Point3D curr = evaluateSegment(segment, u);
            length += pointDistance(prev, curr);
            prev = curr;
        }
        return length;
    }

    static double estimateMaxCurvature(const TrajectorySegment& segment)
    {
        if (segment.type == TrajectorySegmentType::Line) {
            return 0.0;
        }

        double maxCurvature = 0.0;
        constexpr int sampleCount = 32;
        for (int i = 0; i <= sampleCount; ++i) {
            const double u = static_cast<double>(i) / sampleCount;
            const double kappa = segmentCurvature(segment, u);
            if (std::isfinite(kappa)) {
                maxCurvature = std::max(maxCurvature, kappa);
            }
        }
        return maxCurvature;
    }

    static double mergeFeedrate(double a, double b)
    {
        if (a > 0.0 && b > 0.0) {
            return std::min(a, b);
        }
        return a > 0.0 ? a : b;
    }

    static Point3D evalClampedCubicBSpline(const std::array<Point3D, 5>& ctrl, double u)
    {
        if (u <= 0.0) {
            return ctrl.front();
        }
        if (u >= 1.0) {
            return ctrl.back();
        }

        constexpr int degree = 3;
        constexpr int n = 4;
        constexpr std::array<double, 9> knots = {
            0.0, 0.0, 0.0, 0.0, 0.5, 1.0, 1.0, 1.0, 1.0
        };

        int span = degree;
        while (span < n && u >= knots[span + 1]) {
            ++span;
        }

        std::array<Point3D, degree + 1> d{};
        for (int j = 0; j <= degree; ++j) {
            d[j] = ctrl[span - degree + j];
        }

        for (int r = 1; r <= degree; ++r) {
            for (int j = degree; j >= r; --j) {
                const int knotIndex = span - degree + j;
                const double denom = knots[knotIndex + degree - r + 1] - knots[knotIndex];
                const double alpha = denom > 1e-12 ? (u - knots[knotIndex]) / denom : 0.0;
                d[j] = pointAdd(pointScale(d[j - 1], 1.0 - alpha), pointScale(d[j], alpha));
            }
        }
        return d[degree];
    }
};

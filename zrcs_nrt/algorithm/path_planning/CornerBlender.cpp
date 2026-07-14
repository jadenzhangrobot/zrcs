#include "algorithm/path_planning/CornerBlender.h"

#include <algorithm>
#include <cmath>

void CornerBlender::buildCornerBlends(const std::vector<Point3D>& raw,
                                      double cornerTol,
                                      double minRemain,
                                      double minChordForBlend,
                                      std::vector<CornerBlendGeom>& corners)
{
    // 对每个内点 raw[i] 尝试构造与入边/出边相切的圆弧
    for (size_t i = 1; i + 1 < raw.size(); ++i) {
        const Point3D vIn = pointNormalize(pointSub(raw[i], raw[i - 1]));   // 入边方向
        const Point3D vOut = pointNormalize(pointSub(raw[i + 1], raw[i]));  // 出边方向
        const double cosTheta = std::clamp(pointDot(vIn, vOut), -1.0, 1.0);
        const double theta = std::acos(cosTheta); // 转角（外偏角）

        constexpr double pi = 3.14159265358979323846;
        // 近共线：collapse 已处理，无需圆角；近折返：几何不稳定，跳过
        if (theta < 0.01 || theta > pi - 0.01) {
            continue;
        }

        const double lIn = pointDistance(raw[i], raw[i - 1]);
        const double lOut = pointDistance(raw[i + 1], raw[i]);
        // 短弦（曲线离散点）不做局部圆角，否则每个密化点都插弧并留下微残段
        if (std::min(lIn, lOut) < minChordForBlend) {
            continue;
        }

        // 几何：半角 α = θ/2
        // 路径偏差 δ 与截断距 d、半径 R 关系：
        //   R = d / tan(α)
        //   δ = R * (1/cos(α) - 1)  →  d = δ * cos(α)/(1-cos(α))
        const double alpha = theta * 0.5;
        const double cosA = std::cos(alpha);
        const double dTol = (1.0 - cosA) > 1e-9
                              ? cornerTol * cosA / (1.0 - cosA)
                              : 1e6;
        // 两侧各最多用半段，且至少留 minRemain
        const double dMax = std::max(0.0, 0.5 * std::min(lIn, lOut) - std::max(0.0, minRemain));
        const double d = std::min(dTol, dMax);

        if (d < 1e-6) {
            continue;
        }

        // 切入/切出点
        const Point3D start = pointSub(raw[i], pointScale(vIn, d));
        const Point3D end = pointAdd(raw[i], pointScale(vOut, d));
        // 法向与圆心方向
        const Point3D normal = pointNormalize(pointCross(vIn, vOut));
        const Point3D centerDir = pointNormalize(pointCross(normal, vIn)); // 指向弯道内侧
        const double radius = d / std::tan(alpha);
        if (radius < 1e-9 || pointLength(normal) < 1e-9 || pointLength(centerDir) < 1e-9) {
            continue;
        }

        const Point3D center = pointAdd(start, pointScale(centerDir, radius));
        const Point3D basisU = pointNormalize(pointSub(start, center));
        const Point3D basisV = vIn; // 与 basisU 正交的切向基（入边方向）
        corners[i].active = true;
        corners[i].start = start;
        corners[i].end = end;
        corners[i].center = center;
        corners[i].basisU = basisU;
        corners[i].basisV = basisV;
        corners[i].radius = radius;
        corners[i].sweep = theta;
    }
}

std::vector<TrajectorySegment> CornerBlender::fitCornerBlendSegments(
    const std::vector<Point3D>& points,
    const std::vector<double>& feedrates,
    const std::vector<int>& sourceIds,
    double cornerTol,
    double minSegLen,
    double minChordForBlend,
    double rx,
    double ry,
    double rz)
{
    const size_t N = points.size();
    if (N < 2 || feedrates.size() + 1 != N) {
        return {};
    }

    std::vector<CornerBlendGeom> corners(N);
    if (N > 2 && cornerTol > 0.0) {
        // minSegLen 同时作为圆角后两侧最小剩余直线
        buildCornerBlends(points, cornerTol, minSegLen, minChordForBlend, corners);
    }

    auto sourceOf = [&](size_t i) -> int {
        if (i < sourceIds.size()) {
            return sourceIds[i];
        }
        return static_cast<int>(i);
    };

    // 按“直线残段 + 角点圆弧”交错输出
    std::vector<TrajectorySegment> segments;
    int segmentId = 0;

    for (size_t i = 0; i + 1 < N; ++i) {
        const double feed = feedrates[i];
        const int sourceId = sourceOf(i);

        // 直线起点：若本角点有圆角，则从圆角切出点开始
        const Point3D segStart = (i > 0 && corners[i].active)
                                     ? corners[i].endPoint()
                                     : points[i];
        // 直线终点：若下一角点有圆角，则截到圆角切入点
        const Point3D segEnd = (i + 1 < N - 1 && corners[i + 1].active)
                                   ? corners[i + 1].startPoint()
                                   : points[i + 1];

        const double lineLen = pointDistance(segStart, segEnd);
        if (lineLen > 1e-9) {
            segments.push_back(TrajectoryGeometry::makeLineSegment(
                segmentId++, sourceId, segStart, segEnd,
                rx, ry, rz, feed));
        }

        // 在 i+1 角点插入过渡圆弧
        if (i + 1 < N - 1 && corners[i + 1].active) {
            const double cornerFeedrate =
                TrajectoryGeometry::mergeFeedrate(feed, feedrates[i + 1]);
            segments.push_back(TrajectoryGeometry::makeArcSegment(
                segmentId++, sourceId, corners[i + 1],
                rx, ry, rz, cornerFeedrate));
        }
    }

    return segments;
}

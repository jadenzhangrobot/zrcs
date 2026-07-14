#include "algorithm/path_planning/PathSimplifier.h"

#include "algorithm/path_planning/TrajectoryGeometry.h"

#include <algorithm>
#include <cmath>

void PathSimplifier::collapseCollinearWaypoints(std::vector<Point3D>& points,
                                                std::vector<double>& feedrates,
                                                double collinearTol,
                                                std::vector<int>* sourceIds)
{
    // points: N 点；feedrates: N-1 段进给。
    // 折叠近共线中间点 —— 对应 LinuxCNC G64 Q / naive CAM detector。
    // 参考: https://linuxcnc.org/docs/html/gcode/g-code.html#gcode:g64
    if (points.size() < 3 || feedrates.size() + 1 != points.size() || collinearTol <= 0.0) {
        return;
    }
    if (sourceIds && sourceIds->size() != feedrates.size()) {
        // 尺寸不一致时忽略 sourceIds，避免越界
        sourceIds = nullptr;
    }

    std::vector<Point3D> outPts;
    std::vector<double> outFeed;
    std::vector<int> outSrc;
    outPts.reserve(points.size());
    outFeed.reserve(feedrates.size());
    if (sourceIds) {
        outSrc.reserve(sourceIds->size());
    }

    // anchor：当前已确认保留的链起点下标
    outPts.push_back(points.front());
    size_t anchor = 0;
    double runFeed = feedrates.front();
    int runSrc = sourceIds ? (*sourceIds)[0] : 0;

    // 扫描中间点；每个 i 尝试把 [anchor+1 .. i] 都“挂”到弦 [anchor, i+1] 上
    for (size_t i = 1; i + 1 < points.size(); ++i) {
        bool collinear = true;
        for (size_t j = anchor + 1; j <= i; ++j) {
            if (TrajectoryGeometry::pointLineDistance(points[j], points[anchor], points[i + 1]) >
                collinearTol) {
                collinear = false;
                break;
            }
        }

        // 进给变化过大则不能跨段合并（工艺语义）
        const double nextFeed = feedrates[i];
        const bool feedOk = (runFeed <= 0.0 || nextFeed <= 0.0)
                                ? true
                                : std::abs(runFeed - nextFeed) <=
                                      1e-9 * std::max(std::abs(runFeed), std::abs(nextFeed));

        if (collinear && feedOk) {
            // 丢弃 points[i]，延长当前 run
            runFeed = TrajectoryGeometry::mergeFeedrate(runFeed, nextFeed);
            continue;
        }

        // 在 i 处“断链”：输出 run 终点与进给
        outPts.push_back(points[i]);
        outFeed.push_back(runFeed);
        if (sourceIds) {
            outSrc.push_back(runSrc);
        }
        anchor = i;
        runFeed = feedrates[i];
        runSrc = sourceIds ? (*sourceIds)[i] : static_cast<int>(i);
    }

    // 收尾：保留终点
    outPts.push_back(points.back());
    outFeed.push_back(runFeed);
    if (sourceIds) {
        outSrc.push_back(runSrc);
        sourceIds->swap(outSrc);
    }

    points.swap(outPts);
    feedrates.swap(outFeed);
}

void PathSimplifier::mergeShortLineSegments(std::vector<TrajectorySegment>& segments,
                                            double minSegLen,
                                            double collinearTol)
{
    if (segments.size() < 2 || minSegLen <= 0.0) {
        return;
    }

    auto isLine = [](const TrajectorySegment& s) {
        return s.type == TrajectorySegmentType::Line;
    };

    auto lineDir = [](const TrajectorySegment& s) {
        return pointNormalize(pointSub(evaluateSegment(s, 1.0), evaluateSegment(s, 0.0)));
    };

    std::vector<TrajectorySegment> out;
    out.reserve(segments.size());

    for (size_t i = 0; i < segments.size(); ++i) {
        // 圆弧等非直线：原样保留，打断直线合并链
        if (!isLine(segments[i])) {
            out.push_back(segments[i]);
            continue;
        }

        Point3D start = evaluateSegment(segments[i], 0.0);
        Point3D end = evaluateSegment(segments[i], 1.0);
        double feed = segments[i].feedrate_limit;
        const int sourceId = segments[i].source_block_id;
        // 姿态常数存在 coeff[3..5].c0
        const double rx = segments[i].coeff[3][0];
        const double ry = segments[i].coeff[4][0];
        const double rz = segments[i].coeff[5][0];
        Point3D dir = lineDir(segments[i]);

        // 贪心向后吞并后续直线
        size_t j = i;
        while (j + 1 < segments.size() && isLine(segments[j + 1])) {
            const Point3D nextStart = evaluateSegment(segments[j + 1], 0.0);
            const Point3D nextEnd = evaluateSegment(segments[j + 1], 1.0);
            // 几何必须首尾相接
            if (pointDistance(end, nextStart) > 1e-7) {
                break;
            }
            const Point3D nextDir = lineDir(segments[j + 1]);
            const double cosTh = std::clamp(pointDot(dir, nextDir), -1.0, 1.0);
            const double lenSoFar = pointDistance(start, end);
            const bool almostColinear = cosTh > 0.999;              // ~2.5° 内
            const bool shortMerge = lenSoFar < minSegLen && cosTh > 0.98; // 过短且方向尚可
            const double chord = TrajectoryGeometry::pointLineDistance(nextEnd, start, end);
            if (!(almostColinear || shortMerge)) {
                break;
            }
            // 非 shortMerge 时仍要满足弦高容差
            if (collinearTol > 0.0 && chord > collinearTol && !shortMerge) {
                break;
            }
            end = nextEnd;
            feed = TrajectoryGeometry::mergeFeedrate(feed, segments[j + 1].feedrate_limit);
            dir = pointNormalize(pointSub(end, start));
            ++j;
        }

        out.push_back(TrajectoryGeometry::makeLineSegment(
            static_cast<int>(out.size()), sourceId, start, end, rx, ry, rz, feed));
        i = j;
    }

    // 丢掉数值零长段
    segments.clear();
    for (auto& s : out) {
        if (s.length > 1e-9) {
            segments.push_back(std::move(s));
        }
    }
}

#include "algorithm/path_planning/MotionPlanner.h"

#include "algorithm/path_planning/CornerBlender.h"
#include "algorithm/path_planning/PathSimplifier.h"
#include "algorithm/path_planning/ShortSegmentMerger.h"

#include <algorithm>

std::vector<TrajectorySegment> MotionPlanner::buildGeometry(
    const std::vector<PathMoveBlock>& blocks,
    double cornerTol,
    double collinearTol,
    double minSegLen)
{
    // 只收有效直线块（过滤零长）
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

    // 默认与工业控制器常见设定对齐：
    // - collinearTol ≈ cornerTol（G64 Q 常与 P 同量级）
    // - minSegLen 必须明显大于“一个控制周期 × 进给”量级，否则 Ruckig 会在微段上超调回退
    if (collinearTol < 0.0) {
        collinearTol = cornerTol;
    }
    if (minSegLen < 0.0) {
        minSegLen = std::max(1e-6, 3.0 * cornerTol);
    }

    // 展开为点列 + 段属性（进给、源 id）
    std::vector<Point3D> raw;
    std::vector<double> feedrates;
    std::vector<int> sourceIds;
    raw.reserve(linearBlocks.size() + 1);
    feedrates.reserve(linearBlocks.size());
    sourceIds.reserve(linearBlocks.size());
    raw.push_back(linearBlocks.front().start);
    for (const auto& block : linearBlocks) {
        raw.push_back(block.end);
        feedrates.push_back(block.feedrate);
        sourceIds.push_back(block.block_id);
    }

    // ① 路径简化：共线折叠
    PathSimplifier::collapseCollinearWaypoints(raw, feedrates, collinearTol, &sourceIds);
    if (raw.size() < 2) {
        return {};
    }

    // 当前姿态取首块常数（整段路径统一姿态）
    const double rx = linearBlocks.front().rx;
    const double ry = linearBlocks.front().ry;
    const double rz = linearBlocks.front().rz;
    // 短于 4*minSegLen 的弦不做圆角（曲线密化点保护）
    const double minChordForBlend = 4.0 * minSegLen;

    // ② 拐角处理：圆弧过渡
    auto segments = CornerBlender::fitCornerBlendSegments(
        raw, feedrates, sourceIds, cornerTol, minSegLen, minChordForBlend, rx, ry, rz);

    // ③ 短段合并：清圆角后微残段
    ShortSegmentMerger::Config mergeCfg;
    mergeCfg.minSegLen = minSegLen;
    mergeCfg.collinearTol = collinearTol;
    return ShortSegmentMerger::merge(std::move(segments), mergeCfg);
}

bool MotionPlanner::plan(const std::vector<Point3D>& waypoints,
                         double rx,
                         double ry,
                         double rz,
                         const Config& cfg,
                         std::vector<TrajectorySegment>& segments,
                         std::string* error)
{
    segments.clear();

    if (waypoints.size() < 2) {
        if (error) {
            *error = "Need at least 2 waypoints";
        }
        return false;
    }

    // waypoints → 直线块（进给暂用 maxVel，后续可由 NC/端口细化）
    auto blocks = makeBlocksFromWaypoints(waypoints, rx, ry, rz, cfg.maxVel);

    // ①–③ 几何
    segments = buildGeometry(blocks, cfg.cornerTol, cfg.collinearTol, cfg.minSegLen);
    if (segments.empty()) {
        if (error) {
            *error = "Path fitting produced no trajectory segments";
        }
        return false;
    }

    // ④ 速度前瞻
    lookAhead_.setConfig(cfg.maxVel, cfg.maxAccel, cfg.startVel, cfg.endVel, cfg.cornerTol, cfg.maxJerk);
    if (!lookAhead_.planSegments(segments)) {
        if (error) {
            *error = "Segment velocity planning failed";
        }
        segments.clear();
        return false;
    }

    return true;
}

std::vector<PathMoveBlock> MotionPlanner::makeBlocksFromWaypoints(
    const std::vector<Point3D>& waypoints,
    double rx,
    double ry,
    double rz,
    double feedrate)
{
    std::vector<PathMoveBlock> blocks;
    blocks.reserve(waypoints.size() - 1);
    for (size_t i = 1; i < waypoints.size(); ++i) {
        PathMoveBlock block;
        block.block_id = static_cast<int>(i - 1);
        block.type = PathMoveType::Line;
        block.start = waypoints[i - 1];
        block.end = waypoints[i];
        block.rx = rx;
        block.ry = ry;
        block.rz = rz;
        block.feedrate = feedrate;
        blocks.push_back(block);
    }
    return blocks;
}

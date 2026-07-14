#include "algorithm/path_planning/ShortSegmentMerger.h"

#include "algorithm/path_planning/PathSimplifier.h"

std::vector<TrajectorySegment> ShortSegmentMerger::merge(
    std::vector<TrajectorySegment> segments,
    const Config& cfg)
{
    // 委托 PathSimplifier 的段级合并实现，避免两处规则不一致
    if (cfg.minSegLen > 0.0) {
        PathSimplifier::mergeShortLineSegments(segments, cfg.minSegLen, cfg.collinearTol);
    }

    // 统一重编号，便于下游日志与调试
    for (size_t i = 0; i < segments.size(); ++i) {
        segments[i].segment_id = static_cast<int>(i);
    }
    return segments;
}

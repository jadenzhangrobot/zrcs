#pragma once

/// @file ShortSegmentMerger.h
/// @brief 短段合并（流水线第 ③ 步）。
///
/// 圆角后可能留下过短直线残段；本阶段把过短/近共线直线合并，
/// 并重编号 segment_id，保证后续 OTG/前瞻输入干净。
///
/// 注意：这不是样条平滑，而是几何清理。

#include "algorithm/path_planning/TrajectoryTypes.h"

#include <vector>

class ShortSegmentMerger {
public:
    struct Config {
        double minSegLen = 0.0;     ///< 短于此长度且方向接近时允许强制合并
        double collinearTol = 0.0;  ///< 近共线合并的弦高容差
    };

    /// 合并连续过短/近共线直线，返回重编号后的段序列。
    static std::vector<TrajectorySegment> merge(std::vector<TrajectorySegment> segments,
                                                const Config& cfg);
};

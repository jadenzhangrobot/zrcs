#pragma once

/// @file CornerBlender.h
/// @brief 拐角处理（流水线第 ② 步，对应 LinuxCNC G64 P / 容差圆角）。
///
/// 在真拐角处插入圆弧过渡：
/// - 路径偏离原角点不超过 cornerTol
/// - 两侧直线至少保留 minSegLen，避免圆角吃成微残段
/// - 过短弦（曲线密化点）不做圆角，防止每个微折都插弧

#include "algorithm/path_planning/TrajectoryGeometry.h"
#include "algorithm/path_planning/TrajectoryTypes.h"

#include <vector>

class CornerBlender {
public:
    /// 在已简化点列上生成 line/arc 段序列。
    ///
    /// @param points           已简化路径点（N 个）
    /// @param feedrates        N-1 段进给
    /// @param sourceIds        可选源 block id；尺寸不足时回退为段索引
    /// @param cornerTol        角点路径偏差容差 (m)
    /// @param minSegLen        圆角后两侧至少保留的直线长度
    /// @param minChordForBlend 入/出弦短于该值则跳过圆角
    /// @param rx,ry,rz         姿态常数
    static std::vector<TrajectorySegment> fitCornerBlendSegments(
        const std::vector<Point3D>& points,
        const std::vector<double>& feedrates,
        const std::vector<int>& sourceIds,
        double cornerTol,
        double minSegLen,
        double minChordForBlend,
        double rx,
        double ry,
        double rz);

    /// 计算每个内角点的圆角几何，写入 corners[i]（i 为角点下标）。
    static void buildCornerBlends(const std::vector<Point3D>& raw,
                                  double cornerTol,
                                  double minRemain,
                                  double minChordForBlend,
                                  std::vector<CornerBlendGeom>& corners);
};

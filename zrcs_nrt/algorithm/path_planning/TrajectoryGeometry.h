#pragma once

/// @file TrajectoryGeometry.h
/// @brief 轨迹段几何公共工具：弦高、进给合并、Line/Arc 段构造。
///
/// 被 PathSimplifier / CornerBlender 共用，避免各阶段重复实现段工厂。

#include "algorithm/path_planning/TrajectoryTypes.h"

/// 角点圆弧过渡的几何描述（CornerBlender 输出，再转成 CircularArc 段）。
struct CornerBlendGeom {
    bool active = false;       ///< 该角点是否启用圆角
    Point3D start;             ///< 圆角切入点（入边截断点）
    Point3D end;               ///< 圆角切出点（出边截断点）
    Point3D center;            ///< 圆心
    Point3D basisU;            ///< 起点径向单位向量 center→start
    Point3D basisV;            ///< 切向基，与 basisU 正交，指向入边方向
    double radius = 0.0;       ///< 半径
    double sweep = 0.0;        ///< 扫角 (rad)，等于转角 theta

    const Point3D& startPoint() const { return start; }
    const Point3D& endPoint() const { return end; }
};

class TrajectoryGeometry {
public:
    /// 点到直线 AB 的垂直距离（弦高），用于共线判定。
    static double pointLineDistance(const Point3D& p, const Point3D& a, const Point3D& b);

    /// 合并两段进给：双方均有效时取 min；一方无效取另一方。
    static double mergeFeedrate(double a, double b);

    /// 写入单轴多项式系数。
    static void setAxis(TrajectorySegment& segment,
                        int axis,
                        double c0,
                        double c1,
                        double c2,
                        double c3);

    /// 根据几何填充 length / max_curvature。
    static void finalizeGeometry(TrajectorySegment& segment);

    /// 估计段长：圆弧用 |R*sweep|，直线用端点距离。
    static double estimateLength(const TrajectorySegment& segment);

    /// 估计段最大曲率：直线 0，圆弧 1/R。
    static double estimateMaxCurvature(const TrajectorySegment& segment);

    /// 构造直线段：位置用 c0 + c1*u，姿态写入常数轴。
    static TrajectorySegment makeLineSegment(int segmentId,
                                             int sourceBlockId,
                                             const Point3D& start,
                                             const Point3D& end,
                                             double rx,
                                             double ry,
                                             double rz,
                                             double feedrate);

    /// 构造拐角过渡圆弧段。
    static TrajectorySegment makeArcSegment(int segmentId,
                                            int sourceBlockId,
                                            const CornerBlendGeom& corner,
                                            double rx,
                                            double ry,
                                            double rz,
                                            double feedrate);
};

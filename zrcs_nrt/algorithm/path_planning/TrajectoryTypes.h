#pragma once

/// @file TrajectoryTypes.h
/// @brief 路径规划公共类型与基础几何工具。
///
/// 本文件是 path_planning 的数据契约：
/// - 输入：PathMoveBlock（直线运动块）
/// - 输出：TrajectorySegment（几何段 + 速度规划结果字段）
/// - 工具：点运算、段上位置/切线/曲率求值
///
/// 单位约定（控制器内部 SI，与 5axis / MuJoCo 一致）：
/// - 位置 / 长度：m
/// - 进给 / 速度：m/s
/// - 加速度 / jerk：m/s^2、m/s^3
/// - 姿态 rx/ry/rz：rad（当前按整段常数写入 coeff[3..5]）
///
/// G 代码程序侧仍按标准：G21=mm、G20=inch；NcParser 输出已换算为 m。

#include <algorithm>
#include <cmath>

/// 三维笛卡尔点。
struct Point3D {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

/// 输入运动块类型。当前仅支持直线；圆弧在 NC 解析阶段已离散为折线点。
enum class PathMoveType {
    Line
};

/// 规划输入的一段直线运动（通常由 waypoints 或 NC 折线生成）。
struct PathMoveBlock {
    int block_id = 0;                         ///< 源程序/块编号，便于回溯
    PathMoveType type = PathMoveType::Line;

    Point3D start;                            ///< 起点
    Point3D end;                              ///< 终点

    double rx = 0.0;                          ///< 姿态（当前整段常数）
    double ry = 0.0;
    double rz = 0.0;

    /// 编程进给 (m/s)。<=0 表示后续用全局 maxVel 兜底。
    double feedrate = 0.0;
};

/// 几何段类型。当前管线只生成 Line 与 CircularArc（拐角过渡弧）。
enum class TrajectorySegmentType {
    Line,          ///< 直线段，用 coeff 的 c0/c1 表示
    CircularArc    ///< 圆弧段，用 arc_* 字段表示
};

/// 轨迹几何段 + 速度前瞻结果。
///
/// 几何部分：
/// - Line：coeff[0..2] 为位置三次式（直线时 c2=c3=0），coeff[3..5] 为姿态常数
/// - CircularArc：arc_center/arc_u/arc_v/arc_radius/arc_sweep
///
/// 速度部分（LookAheadPlanner 写入）：
/// - feedrate_limit：段编程进给上限
/// - v_max_local：综合曲率/进给后的局部速度上限
/// - v_enter / v_exit：段入口/出口速度
/// - a_enter / a_exit：段入口/出口切向加速度
/// - duration：规划持续时间（非实时插补用）
struct TrajectorySegment {
    int segment_id = 0;                       ///< 段序号（流水线末尾会重编号）
    int source_block_id = 0;                  ///< 来源 PathMoveBlock::block_id

    TrajectorySegmentType type = TrajectorySegmentType::Line;

    double length = 0.0;                      ///< 弧长 (m)

    /// 轴参数多项式：pos = c0 + c1*u + c2*u^2 + c3*u^3，u∈[0,1]
    /// axis: 0=x,1=y,2=z,3=rx,4=ry,5=rz
    double coeff[6][4] = {};

    Point3D arc_center;                       ///< 圆弧圆心
    Point3D arc_u;                            ///< 圆弧起点方向（单位向量，center→start）
    Point3D arc_v;                            ///< 圆弧切向基（单位向量，与 arc_u 正交）
    double arc_radius = 0.0;                  ///< 半径
    double arc_sweep = 0.0;                   ///< 扫角 (rad)，有符号
    double max_curvature = 0.0;               ///< 段内最大曲率估计

    double feedrate_limit = 0.0;              ///< 编程进给上限
    double v_max_local = 0.0;                 ///< 局部允许最大速度

    double v_enter = 0.0;                     ///< 入口速度
    double v_exit = 0.0;                      ///< 出口速度
    double a_enter = 0.0;                     ///< 入口切向加速度 (m/s^2)
    double a_exit = 0.0;                      ///< 出口切向加速度 (m/s^2)
    double duration = 0.0;                    ///< 规划时长 (s)

    bool is_lookahead_optimized = false;      ///< 是否已完成速度前瞻
};

// ---------------------------------------------------------------------------
// 点运算
// ---------------------------------------------------------------------------

inline Point3D pointAdd(const Point3D& a, const Point3D& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Point3D pointSub(const Point3D& a, const Point3D& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Point3D pointScale(const Point3D& p, double s)
{
    return {p.x * s, p.y * s, p.z * s};
}

inline double pointDot(const Point3D& a, const Point3D& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Point3D pointCross(const Point3D& a, const Point3D& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

inline double pointLength(const Point3D& p)
{
    return std::sqrt(pointDot(p, p));
}

inline double pointDistance(const Point3D& a, const Point3D& b)
{
    return pointLength(pointSub(a, b));
}

/// 单位化；长度过小返回零向量。
inline Point3D pointNormalize(const Point3D& p)
{
    const double len = pointLength(p);
    if (len < 1e-12) {
        return {};
    }
    return pointScale(p, 1.0 / len);
}

// ---------------------------------------------------------------------------
// 段几何求值（u ∈ [0,1]）
// ---------------------------------------------------------------------------

/// 单轴位置：c0 + c1*u + c2*u^2 + c3*u^3
inline double evalSegmentAxis(const TrajectorySegment& seg, int axis, double u)
{
    u = std::clamp(u, 0.0, 1.0);
    const double u2 = u * u;
    const double u3 = u2 * u;
    return seg.coeff[axis][0] +
           seg.coeff[axis][1] * u +
           seg.coeff[axis][2] * u2 +
           seg.coeff[axis][3] * u3;
}

/// 单轴一阶导 d/du（注意：对弧长参数化需再除以 |r'(u)|）。
inline double evalSegmentAxisDerivative(const TrajectorySegment& seg, int axis, double u)
{
    u = std::clamp(u, 0.0, 1.0);
    return seg.coeff[axis][1] +
           2.0 * seg.coeff[axis][2] * u +
           3.0 * seg.coeff[axis][3] * u * u;
}

/// 单轴二阶导 d²/du²。
inline double evalSegmentAxisSecondDerivative(const TrajectorySegment& seg, int axis, double u)
{
    u = std::clamp(u, 0.0, 1.0);
    return 2.0 * seg.coeff[axis][2] +
           6.0 * seg.coeff[axis][3] * u;
}

/// 段上位置。圆弧用极角参数；直线/多项式用 coeff。
inline Point3D evaluateSegment(const TrajectorySegment& seg, double u)
{
    if (seg.type == TrajectorySegmentType::CircularArc) {
        u = std::clamp(u, 0.0, 1.0);
        const double theta = seg.arc_sweep * u;
        const double c = std::cos(theta);
        const double s = std::sin(theta);
        return pointAdd(seg.arc_center,
                        pointScale(pointAdd(pointScale(seg.arc_u, c),
                                            pointScale(seg.arc_v, s)),
                                   seg.arc_radius));
    }

    return {
        evalSegmentAxis(seg, 0, u),
        evalSegmentAxis(seg, 1, u),
        evalSegmentAxis(seg, 2, u),
    };
}

/// 段上切向量（对参数 u 的导数，不是单位切向）。
inline Point3D segmentTangent(const TrajectorySegment& seg, double u)
{
    if (seg.type == TrajectorySegmentType::CircularArc) {
        u = std::clamp(u, 0.0, 1.0);
        const double theta = seg.arc_sweep * u;
        const double c = std::cos(theta);
        const double s = std::sin(theta);
        // d/du [center + R*(u_hat*cos + v_hat*sin)] = R*sweep*(-u_hat*sin + v_hat*cos)
        return pointScale(pointAdd(pointScale(seg.arc_u, -s),
                                   pointScale(seg.arc_v, c)),
                          seg.arc_radius * seg.arc_sweep);
    }

    return {
        evalSegmentAxisDerivative(seg, 0, u),
        evalSegmentAxisDerivative(seg, 1, u),
        evalSegmentAxisDerivative(seg, 2, u),
    };
}

/// 段上二阶导（用于曲率 κ = |r' × r''| / |r'|^3）。
inline Point3D segmentSecondDerivative(const TrajectorySegment& seg, double u)
{
    if (seg.type == TrajectorySegmentType::CircularArc) {
        u = std::clamp(u, 0.0, 1.0);
        const double theta = seg.arc_sweep * u;
        const double c = std::cos(theta);
        const double s = std::sin(theta);
        return pointScale(pointAdd(pointScale(seg.arc_u, -c),
                                   pointScale(seg.arc_v, -s)),
                          seg.arc_radius * seg.arc_sweep * seg.arc_sweep);
    }

    return {
        evalSegmentAxisSecondDerivative(seg, 0, u),
        evalSegmentAxisSecondDerivative(seg, 1, u),
        evalSegmentAxisSecondDerivative(seg, 2, u),
    };
}

/// 曲率标量。速度过小时返回 0，避免数值爆炸。
inline double segmentCurvature(const TrajectorySegment& seg, double u)
{
    const Point3D d1 = segmentTangent(seg, u);
    const Point3D d2 = segmentSecondDerivative(seg, u);
    const double speed = pointLength(d1);
    if (speed < 1e-9) {
        return 0.0;
    }

    return pointLength(pointCross(d1, d2)) / (speed * speed * speed);
}

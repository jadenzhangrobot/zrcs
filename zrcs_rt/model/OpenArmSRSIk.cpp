/**
 * @file OpenArmSRSIk.cpp
 * @brief openArm SRS 解析逆运动学求解器实现
 */

#include "model/OpenArmSRSIk.h"
#include <cmath>
#include <algorithm>
#include <limits>

// ============================================================================
// SRSIkSolution
// ============================================================================

double SRSIkSolution::distanceLInf(const Eigen::Matrix<double, 7, 1>& ref) const
{
    double d = 0.0;
    for (int i = 0; i < 7; i++)
    {
        // std::remainder 直接把角差归一化到 [-pi, pi]。旧实现对
        // fmod(diff + pi, 2*pi) 少减了一个 pi，导致相同角度的距离
        // 反而等于 pi，IK 因此会优先选择最远的翻腕/换肘分支。
        const double diff = std::abs(std::remainder(q(i) - ref(i), 2.0 * M_PI));
        d = std::max(d, diff);
    }
    return d;
}

// ============================================================================
// 构造
// ============================================================================

OpenArmSRSIk::OpenArmSRSIk(Side side)
    : m_side(side)
{
    const double ySign = (side == Side::LEFT) ? 1.0 : -1.0;

    // 关节轴 (自身坐标系)
    m_axes[0] = { 0.0,  ySign,  0.0};
    m_axes[1] = {-1.0,  0.0,    0.0};
    m_axes[2] = { 0.0,  0.0,   -1.0};
    m_axes[3] = { 0.0, -1.0,    0.0};   // 肘部
    m_axes[4] = { 0.0,  0.0,   -1.0};
    m_axes[5] = { 0.0, -1.0,    0.0};
    m_axes[6] = { 1.0,  0.0,    0.0};

    // T_left 平移
    m_T_left_trans[0] = {0.0,  0.0625 * ySign,  0.0};
    m_T_left_trans[1] = {0.0,  0.06   * ySign,  0.0};
    m_T_left_trans[2] = {0.0,  0.0,            -0.06625};
    m_T_left_trans[3] = {0.0,  0.0,            -0.15375};
    m_T_left_trans[4] = {0.0,  0.0,            -0.0955};
    m_T_left_trans[5] = {0.0,  0.0,            -0.1205};
    m_T_left_trans[6] = {0.0,  0.0,             0.0};

    // 关节限位 (与 axis.xml 一致)
    if (side == Side::LEFT)
    {
        m_limitLower = {-3.4907, -3.3161, -1.5708,  0.0,    -1.5708, -0.7854, -0.7854};
        m_limitUpper = { 1.3963,  0.17453, 1.5708,  2.4435,  1.5708,  0.7854,  0.7854};
    }
    else
    {
        m_limitLower = {-1.3963, -0.17453,-1.5708,  0.0,    -1.5708, -0.7854, -0.7854};
        m_limitUpper = { 3.4907,  3.3161,  1.5708,  2.4435,  1.5708,  0.7854,  0.7854};
    }

    computeOrigins();

    // SRS 几何常量
    m_shoulderPivot = {0.0, 0.1225 * ySign, 0.0};
    m_wristPivot    = {0.0, 0.1225 * ySign, -0.436};

    const Eigen::Vector3d elbowOrigin = m_origins[3];

    m_upperHome = elbowOrigin - m_shoulderPivot;
    m_L_se = m_upperHome.norm();

    m_forearmHome = m_wristPivot - elbowOrigin;
    m_L_ew = m_forearmHome.norm();

    m_eeOffsetLocal = Eigen::Vector3d::Zero();
}

void OpenArmSRSIk::computeOrigins()
{
    Eigen::Matrix3d R = Eigen::Matrix3d::Identity();
    Eigen::Vector3d p(0, 0, 0);

    for (int i = 0; i < DOF; i++)
    {
        p += R * m_T_left_trans[i];
        m_origins[i] = p;
        // At q=0, joints don't rotate, T_right is identity
    }
}

// ============================================================================
// 正运动学 (POE)
// ============================================================================

Eigen::Matrix4d OpenArmSRSIk::forwardKinematics(
    const Eigen::Matrix<double, 7, 1>& q) const
{
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();

    for (int i = 0; i < DOF; i++)
    {
        T.block<3,1>(0,3) += T.block<3,3>(0,0) * m_T_left_trans[i];

        Eigen::Matrix3d R_j = axisAngleMatrix(m_axes[i], q(i));
        T.block<3,3>(0,0) = T.block<3,3>(0,0) * R_j;
    }

    return T;
}

// ============================================================================
// Jacobian
// ============================================================================

void OpenArmSRSIk::frameAtJoint(const Eigen::Matrix<double, 7, 1>& q,
                                 int jointIdx,
                                 Eigen::Matrix3d& R,
                                 Eigen::Vector3d& p) const
{
    R = Eigen::Matrix3d::Identity();
    p = Eigen::Vector3d::Zero();

    for (int i = 0; i < DOF; i++)
    {
        p += R * m_T_left_trans[i];
        if (i == jointIdx)
            return;
        R = R * axisAngleMatrix(m_axes[i], q(i));
    }
}

Eigen::Matrix<double, 6, 7> OpenArmSRSIk::jacobian(
    const Eigen::Matrix<double, 7, 1>& q) const
{
    Eigen::Matrix<double, 6, 7> J;

    // 累积变换
    std::array<Eigen::Matrix3d, DOF + 1> R_cum;
    std::array<Eigen::Vector3d, DOF + 1>  p_cum;
    R_cum[0] = Eigen::Matrix3d::Identity();
    p_cum[0] = Eigen::Vector3d::Zero();

    for (int i = 0; i < DOF; i++)
    {
        p_cum[i + 1] = p_cum[i] + R_cum[i] * m_T_left_trans[i];
        R_cum[i + 1] = R_cum[i] * axisAngleMatrix(m_axes[i], q(i));
    }

    // 末端位置
    Eigen::Vector3d pn = p_cum[DOF];

    for (int i = 0; i < DOF; i++)
    {
        // 关节 i 在世界系中的 z 轴（旋转轴）
        Eigen::Vector3d zi = R_cum[i] * m_axes[i];
        Eigen::Vector3d pi = p_cum[i] + R_cum[i] * m_T_left_trans[i];

        // 线速度分量: z_i × (p_n - p_i)
        J.block<3,1>(0, i) = zi.cross(pn - pi);
        // 角速度分量: z_i
        J.block<3,1>(3, i) = zi;
    }

    return J;
}

// ============================================================================
// 逆运动学 (Singh-Kreutz SRS)
// ============================================================================

std::vector<SRSIkSolution> OpenArmSRSIk::solve(
    const Eigen::Matrix4d& T_target,
    const Eigen::Matrix<double, 7, 1>& q_ref,
    int maxSolutions,
    int swivelSamples,
    bool respectLimits) const
{
    std::vector<SRSIkSolution> solutions;

    const Eigen::Matrix3d R_target = T_target.block<3,3>(0,0);
    const Eigen::Vector3d p_target = T_target.block<3,1>(0,3);

    // ================================================================
    // Step 1: 目标腕部位置
    // ================================================================
    const Eigen::Vector3d W_t = p_target - R_target * m_eeOffsetLocal;

    // ================================================================
    // Step 2: 肩-腕可达性检查
    // ================================================================
    const Eigen::Vector3d SW = W_t - m_shoulderPivot;
    const double d_sw = SW.norm();

    if (d_sw > m_L_se + m_L_ew + 1e-6)
        return solutions;
    if (d_sw < std::abs(m_L_se - m_L_ew) - 1e-6)
        return solutions;
    if (d_sw < 1e-12)  // 完全伸展/折叠时 swivel 退化为点
        return solutions;

    const Eigen::Vector3d u_sw = SW / d_sw;

    // ================================================================
    // Step 3: 肘角 q₃ (余弦定理, 2 分支)
    // ================================================================
    const double cos_q3 = std::clamp(
        (m_L_se * m_L_se + m_L_ew * m_L_ew - d_sw * d_sw) / (2.0 * m_L_se * m_L_ew),
        -1.0, 1.0);
    const double base_q3 = M_PI - std::acos(cos_q3);
    const std::array<double, 2> q3_branches = {base_q3, -base_q3};

    // ================================================================
    // Step 4: Swivel 圆参数
    // ================================================================
    const double x_c = (m_L_se * m_L_se - m_L_ew * m_L_ew + d_sw * d_sw) / (2.0 * d_sw);
    const double r_circle_sq = std::max(m_L_se * m_L_se - x_c * x_c, 0.0);
    const double r_circle = std::sqrt(r_circle_sq);

    auto [u_p1, u_p2] = swivelBasis(u_sw);

    // 关节轴方向 (自身坐标系, 已归一化)
    const Eigen::Vector3d n3 = m_axes[3];  // 肘轴
    const Eigen::Vector3d n0 = m_axes[0];
    const Eigen::Vector3d n1 = m_axes[1];
    const Eigen::Vector3d n2 = m_axes[2];
    const Eigen::Vector3d n4 = m_axes[4];
    const Eigen::Vector3d n5 = m_axes[5];
    const Eigen::Vector3d n6 = m_axes[6];

    // 上臂单位方向 (q=0, 基座系)
    const Eigen::Vector3d u_home = m_upperHome / m_L_se;

    // ================================================================
    // Step 5: 遍历 swivel 角度
    // ================================================================
    for (int si = 0; si < swivelSamples; si++)
    {
        const double psi = -M_PI + (2.0 * M_PI * si) / swivelSamples;

        // 肘部位置 E_t 在 swivel 圆上
        const Eigen::Vector3d E_t = m_shoulderPivot
            + x_c * u_sw
            + r_circle * (std::cos(psi) * u_p1 + std::sin(psi) * u_p2);

        const Eigen::Vector3d upper = E_t - m_shoulderPivot;
        const Eigen::Vector3d d_hat = upper / m_L_se;

        // R0: 将 home 上臂方向映射到 d_hat 的最小旋转
        const Eigen::Matrix3d R0 = minRotation(u_home, upper);

        const Eigen::Vector3d wrist_vec = W_t - E_t;

        // 肘轴在世界系中 (经 R0)
        const Eigen::Vector3d k_elbow_world = R0 * n3;
        const Eigen::Vector3d v_forearm0 = R0 * m_forearmHome;

        // ------------------------------------------------------------
        // 5a: 肘角 q₃ via SP4
        // ------------------------------------------------------------
        for (double q3 : solveSP4(d_hat, k_elbow_world, v_forearm0,
                                   wrist_vec.dot(d_hat)))
        {
            // 前臂方向 (经 R0 旋转, 经 q3 折弯, 但未经肩部滚动 φ)
            const Eigen::Vector3d g = R0 * (axisAngleMatrix(n3, q3) * m_forearmHome);

            // ------------------------------------------------------------
            // 5b: 肩部滚动角 φ via SP1
            // ------------------------------------------------------------
            const Eigen::Vector3d g_perp = g - d_hat * d_hat.dot(g);
            const Eigen::Vector3d w_perp = wrist_vec - d_hat * d_hat.dot(wrist_vec);

            double phi = 0.0;
            const double g_perp_norm = g_perp.norm();
            if (g_perp_norm > 1e-9)
            {
                phi = std::atan2(d_hat.dot(g_perp.cross(w_perp)),
                                 g_perp.dot(w_perp));
            }

            // 肩部旋转 = roll(phi) @ R0
            const Eigen::Matrix3d R_sh = axisAngleMatrix(d_hat, phi) * R0;

            // 腕部残差旋转
            const Eigen::Matrix3d R_pre_elbow =
                R_sh * axisAngleMatrix(n3, q3);
            const Eigen::Matrix3d R_res =
                R_pre_elbow.transpose() * R_target;

            // ------------------------------------------------------------
            // 5c: 肩关节 (q₀, q₁, q₂) — 广义三轴分解
            // ------------------------------------------------------------
            for (auto& sq : decompose3Axis(R_sh, n0, n1, n2))
            {
                // ------------------------------------------------------------
                // 5d: 腕关节 (q₄, q₅, q₆) — 广义三轴分解
                // ------------------------------------------------------------
                for (auto& wq : decompose3Axis(R_res, n4, n5, n6))
                {
                    SRSIkSolution sol;
                    sol.q << sq[0], sq[1], sq[2], q3, wq[0], wq[1], wq[2];

                    if (!sol.q.allFinite())
                        continue;

                    // 关节限位
                    if (respectLimits)
                    {
                        sol.inLimits = true;
                        for (int j = 0; j < DOF; j++)
                        {
                            if (sol.q(j) < m_limitLower[j] - 1e-8 ||
                                sol.q(j) > m_limitUpper[j] + 1e-8)
                            {
                                sol.inLimits = false;
                                break;
                            }
                        }
                        if (!sol.inLimits)
                            continue;
                    }

                    // FK 验证
                    const Eigen::Matrix4d T_fk = forwardKinematics(sol.q);
                    sol.fkResidual = (T_fk - T_target).norm();
                    if (sol.fkResidual > 1e-5)
                        continue;

                    solutions.push_back(std::move(sol));
                }
            }
        }
    }

    // ================================================================
    // 后处理: 去重 + 按距参考构型排序
    // ================================================================
    if (solutions.size() > 1)
    {
        // 去重 (关节角在 ±π wrap 内去重)
        std::vector<SRSIkSolution> deduped;
        constexpr double kDedupTol = 5e-3;
        for (auto& sol : solutions)
        {
            bool dup = false;
            for (auto& d : deduped)
            {
                if ((sol.q - d.q).cwiseAbs().maxCoeff() < kDedupTol)
                {
                    dup = true;
                    break;
                }
            }
            if (!dup)
                deduped.push_back(std::move(sol));
        }
        solutions = std::move(deduped);

        // 排序
        std::sort(solutions.begin(), solutions.end(),
            [&](const SRSIkSolution& a, const SRSIkSolution& b)
            {
                return a.distanceLInf(q_ref) < b.distanceLInf(q_ref);
            });
    }

    if (maxSolutions > 0 && static_cast<int>(solutions.size()) > maxSolutions)
        solutions.resize(maxSolutions);

    return solutions;
}

// ============================================================================
// 轨迹跟踪
// ============================================================================

SRSIkSolution OpenArmSRSIk::track(
    const Eigen::Matrix<double, 7, 1>& q_seed,
    const Eigen::Matrix4d& T_target,
    int maxIters,
    double tolerance) const
{
    SRSIkSolution result;
    result.q = q_seed;

    for (int iter = 0; iter < maxIters; iter++)
    {
        Eigen::Matrix4d T_cur = forwardKinematics(result.q);

        Eigen::Vector3d pos_err = T_target.block<3,1>(0,3) - T_cur.block<3,1>(0,3);

        Eigen::Matrix3d R_err = T_target.block<3,3>(0,0)
                              * T_cur.block<3,3>(0,0).transpose();
        Eigen::AngleAxisd aa(R_err);
        Eigen::Vector3d ori_err = aa.angle() * aa.axis();

        Eigen::Matrix<double, 6, 1> error;
        error << pos_err, ori_err;

        if (error.norm() < tolerance)
        {
            result.fkResidual = (T_cur - T_target).norm();
            result.inLimits = isWithinLimits(result.q);
            if (result.inLimits && result.fkResidual <= 1e-5)
                return result;
            break;
        }

        Eigen::Matrix<double, 6, 7> J = jacobian(result.q);
        Eigen::Matrix<double, 6, 6> JJt = J * J.transpose()
            + 1e-3 * Eigen::Matrix<double, 6, 6>::Identity();
        result.q += J.transpose() * JJt.ldlt().solve(error);
        if (!result.q.allFinite())
            break;
    }

    result.inLimits = false;
    result.q.setConstant(std::numeric_limits<double>::quiet_NaN());
    return result;
}

bool OpenArmSRSIk::isWithinLimits(
    const Eigen::Matrix<double, 7, 1>& q,
    double margin) const
{
    if (!q.allFinite() || margin < 0.0)
        return false;

    for (int i = 0; i < DOF; ++i)
    {
        if (q(i) < m_limitLower[i] + margin ||
            q(i) > m_limitUpper[i] - margin)
            return false;
    }
    return true;
}

double OpenArmSRSIk::minimumLimitMargin(
    const Eigen::Matrix<double, 7, 1>& q) const
{
    if (!q.allFinite())
        return -std::numeric_limits<double>::infinity();

    double minimum = std::numeric_limits<double>::infinity();
    for (int i = 0; i < DOF; ++i)
    {
        minimum = std::min(
            minimum,
            std::min(q(i) - m_limitLower[i], m_limitUpper[i] - q(i)));
    }
    return minimum;
}

Eigen::Matrix<double, 7, 1> OpenArmSRSIk::clampToLimits(
    const Eigen::Matrix<double, 7, 1>& q) const
{
    Eigen::Matrix<double, 7, 1> clamped = q;
    for (int i = 0; i < DOF; ++i)
    {
        if (!std::isfinite(clamped(i)))
            continue;
        clamped(i) = std::clamp(clamped(i), m_limitLower[i], m_limitUpper[i]);
    }
    return clamped;
}

// ============================================================================
// 辅助函数
// ============================================================================

Eigen::Matrix3d OpenArmSRSIk::minRotation(const Eigen::Vector3d& u,
                                           const Eigen::Vector3d& v)
{
    Eigen::Vector3d u_n = u.normalized();
    Eigen::Vector3d v_n = v.normalized();
    double c = u_n.dot(v_n);

    if (c > 1.0 - 1e-12)
        return Eigen::Matrix3d::Identity();

    if (c < -1.0 + 1e-12)
    {
        // 反向: 绕任意垂直于 u 的轴转 π
        Eigen::Vector3d perp = u_n.cross(Eigen::Vector3d::UnitX());
        if (perp.norm() < 1e-6)
            perp = u_n.cross(Eigen::Vector3d::UnitY());
        return axisAngleMatrix(perp.normalized(), M_PI);
    }

    Eigen::Vector3d axis = u_n.cross(v_n);
    return axisAngleMatrix(axis.normalized(), std::acos(c));
}

Eigen::Matrix3d OpenArmSRSIk::axisAngleMatrix(const Eigen::Vector3d& axis,
                                               double angle)
{
    double c = std::cos(angle);
    double s = std::sin(angle);
    double t = 1.0 - c;

    double ax = axis.x(), ay = axis.y(), az = axis.z();

    Eigen::Matrix3d R;
    R << t*ax*ax + c,     t*ax*ay - s*az,   t*ax*az + s*ay,
         t*ax*ay + s*az,  t*ay*ay + c,      t*ay*az - s*ax,
         t*ax*az - s*ay,  t*ay*az + s*ax,   t*az*az + c;
    return R;
}

std::vector<double> OpenArmSRSIk::solveSP4(
    const Eigen::Vector3d& h,
    const Eigen::Vector3d& k,
    const Eigen::Vector3d& p,
    double delta)
{
    // SP4: h · (Rot(k, q) p) = delta
    //     A cos q + B sin q = C
    double hk = h.dot(k);
    double kp = k.dot(p);

    double A = h.dot(p) - hk * kp;
    double B = h.dot(k.cross(p));
    double C = delta - hk * kp;

    double amp = std::hypot(A, B);
    if (amp < 1e-12)
        return {};

    double ratio = C / amp;
    if (std::abs(ratio) > 1.0 + 1e-9)
        return {};

    double base = std::atan2(B, A);
    double off  = std::acos(std::clamp(ratio, -1.0, 1.0));

    if (off < 1e-12)
        return {base + off};
    else
        return {base + off, base - off};
}

std::vector<std::array<double, 3>> OpenArmSRSIk::decompose3Axis(
    const Eigen::Matrix3d& R,
    const Eigen::Vector3d& n0,
    const Eigen::Vector3d& n1,
    const Eigen::Vector3d& n2)
{
    // ================================================================
    // 广义三轴 Davenport 分解: R = Rot(n0,q0) · Rot(n1,q1) · Rot(n2,q2)
    //
    // n0, n1, n2 都是 cardinal 方向 (±X, ±Y, ±Z) 且两两正交。
    //
    // 思路:
    //   每个 n_i = s_i · e_{p_i}, 其中 s_i=±1, p_i∈{0=X,1=Y,2=Z}
    //   R = Rot(s0·e_{p0}, q0) · Rot(s1·e_{p1}, q1) · Rot(s2·e_{p2}, q2)
    //     = Rot(e_{p0}, s0·q0) · Rot(e_{p1}, s1·q1) · Rot(e_{p2}, s2·q2)
    //
    // 令 a=s0·q0, b=s1·q1, c=s2·q2, 则 R = Rot(e_{p0},a)·Rot(e_{p1},b)·Rot(e_{p2},c)
    // 这是标准轴外旋欧拉角 (顺序 {p0, p1, p2}), 用对应公式提取 a,b,c,
    // 再映射回 q0=s0·a, q1=s1·b, q2=s2·c。
    //
    // openArm 轴序:
    //   肩部: n0→Y, n1→-X, n2→-Z  → 顺序 {Y, X, Z} = YXZ
    //   腕部: n4→-Z, n5→-Y, n6→X  → 顺序 {Z, Y, X} = ZYX
    // ================================================================

    std::vector<std::array<double, 3>> result;

    // 正交性检查
    if (std::abs(n0.dot(n1)) > 1e-9 ||
        std::abs(n1.dot(n2)) > 1e-9 ||
        std::abs(n0.dot(n2)) > 1e-9)
        return result;

    // 识别每根轴的 cardinal 方向和符号
    struct AxisInfo { double s; int axis; };  // axis: 0=X, 1=Y, 2=Z
    AxisInfo sa[3];
    const Eigen::Vector3d* axes[3] = {&n0, &n1, &n2};

    for (int i = 0; i < 3; i++)
    {
        const Eigen::Vector3d& v = *axes[i];
        if      (std::abs(v.x()) > 0.9) sa[i] = {(v.x() > 0 ? 1.0 : -1.0), 0};
        else if (std::abs(v.y()) > 0.9) sa[i] = {(v.y() > 0 ? 1.0 : -1.0), 1};
        else if (std::abs(v.z()) > 0.9) sa[i] = {(v.z() > 0 ? 1.0 : -1.0), 2};
        else return result;
    }

    // 确保 {p0, p1, p2} 是 {0,1,2} 的排列
    int seen = (1 << sa[0].axis) | (1 << sa[1].axis) | (1 << sa[2].axis);
    if (seen != 0b111) return result;

    int p0 = sa[0].axis, p1 = sa[1].axis, p2 = sa[2].axis;
    double s0 = sa[0].s, s1 = sa[1].s, s2 = sa[2].s;

    // ----------------------------------------------------------------
    // 根据轴排列 (p0,p1,p2) 选择对应的欧拉角提取公式
    // 公式: R = Rot(e_{p0},a) · Rot(e_{p1},b) · Rot(e_{p2},c)
    //
    // 以旋转矩阵元素直接索引，对每种排列硬编码提取逻辑。
    // ----------------------------------------------------------------

    // 辅助: 从 (sin, cos) 中提取两分支角度，wrap 到 [-π,π]
    auto twoBranches = [](double sinVal, double cosHint) -> std::array<double, 2>
    {
        double b0 = std::atan2(sinVal, cosHint);
        double b1 = b0 > 0 ? b0 - M_PI : b0 + M_PI;
        // wrap
        if (b0 > M_PI) b0 -= 2*M_PI; else if (b0 < -M_PI) b0 += 2*M_PI;
        if (b1 > M_PI) b1 -= 2*M_PI; else if (b1 < -M_PI) b1 += 2*M_PI;
        return {b0, b1};
    };

    // R 的矩阵元素 (0-indexed)
    auto r = [&R](int row, int col) { return R(row, col); };

    // 根据排列计算候选角度三元组
    std::vector<std::array<double, 3>> candidates;

    if (p0 == 1 && p1 == 0 && p2 == 2)
    {
        // ---- YXZ: R = Rot(Y,a)·Rot(X,b)·Rot(Z,c) ----
        double sin_b = -r(1, 2);
        for (double b : twoBranches(sin_b, std::sqrt(std::max(1.0 - sin_b*sin_b, 0.0))))
        {
            double cb = std::cos(b);
            double a, c;
            if (std::abs(cb) > 1e-9)
            {
                a = std::atan2(r(0,2)/cb, r(2,2)/cb);
                c = std::atan2(r(1,0)/cb, r(1,1)/cb);
            }
            else
            {
                a = 0.0;
                c = (b > 0) ? std::atan2(r(2,0), r(2,1))
                            : std::atan2(-r(2,0), -r(2,1));
            }
            candidates.push_back({a, b, c});
        }
    }
    else if (p0 == 2 && p1 == 1 && p2 == 0)
    {
        // ---- ZYX: R = Rot(Z,a)·Rot(Y,b)·Rot(X,c) ----
        double sin_b = -r(2, 0);
        for (double b : twoBranches(sin_b, std::sqrt(std::max(1.0 - sin_b*sin_b, 0.0))))
        {
            double cb = std::cos(b);
            double a, c;
            if (std::abs(cb) > 1e-9)
            {
                a = std::atan2(r(1,0)/cb, r(0,0)/cb);
                c = std::atan2(r(2,1)/cb, r(2,2)/cb);
            }
            else
            {
                a = 0.0;
                c = (b > 0) ? std::atan2(r(0,1), r(0,2))
                            : std::atan2(-r(0,1), -r(0,2));
            }
            candidates.push_back({a, b, c});
        }
    }
    else if (p0 == 0 && p1 == 1 && p2 == 2)
    {
        // ---- XYZ: R = Rot(X,a)·Rot(Y,b)·Rot(Z,c) ----
        double sin_b = r(0, 2);
        for (double b : twoBranches(sin_b, std::sqrt(std::max(1.0 - sin_b*sin_b, 0.0))))
        {
            double cb = std::cos(b);
            double a, c;
            if (std::abs(cb) > 1e-9)
            {
                a = std::atan2(-r(1,2)/cb, r(2,2)/cb);
                c = std::atan2(-r(0,1)/cb, r(0,0)/cb);
            }
            else
            {
                a = 0.0;
                c = (b > 0) ? std::atan2(r(1,0), r(1,1))
                            : std::atan2(-r(1,0), -r(1,1));
            }
            candidates.push_back({a, b, c});
        }
    }
    else
    {
        return result;  // 未支持的排列
    }

    // 映射回实际关节角: q0=s0·a, q1=s1·b, q2=s2·c, 并用 FK 验证
    for (auto& abc : candidates)
    {
        double q0 = s0 * abc[0];
        double q1 = s1 * abc[1];
        double q2 = s2 * abc[2];

        Eigen::Matrix3d R_test = axisAngleMatrix(n0, q0)
                               * axisAngleMatrix(n1, q1)
                               * axisAngleMatrix(n2, q2);

        if ((R_test - R).norm() < 1e-6)
            result.push_back({q0, q1, q2});
    }

    // 去重
    std::vector<std::array<double, 3>> deduped;
    for (auto& sol : result)
    {
        bool dup = false;
        for (auto& d : deduped)
        {
            if (std::abs(sol[0] - d[0]) < 1e-4 &&
                std::abs(sol[1] - d[1]) < 1e-4 &&
                std::abs(sol[2] - d[2]) < 1e-4)
            { dup = true; break; }
        }
        if (!dup) deduped.push_back(sol);
    }
    return deduped;
}

std::pair<Eigen::Vector3d, Eigen::Vector3d>
OpenArmSRSIk::swivelBasis(const Eigen::Vector3d& u_sw)
{
    Eigen::Vector3d ref;
    if (std::abs(u_sw.z()) < 0.99)
        ref = Eigen::Vector3d::UnitZ();
    else
        ref = Eigen::Vector3d::UnitX();

    Eigen::Vector3d u_p1 = (ref - ref.dot(u_sw) * u_sw).normalized();
    Eigen::Vector3d u_p2 = u_sw.cross(u_p1).normalized();

    return {u_p1, u_p2};
}

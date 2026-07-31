/**
 * @file OpenArmSRSIk.h
 * @brief openArm 7-DOF 机械臂 SRS 解析逆运动学求解器
 *
 * 基于 Singh-Kreutz (1989) 臂角参数化算法，适配 openArm v2.0
 * 的非标准 ZYZ 肩/腕构型（"non-Z*Z SRS"）。
 *
 * 运动学结构（SRS = Spherical-Revolute-Spherical）：
 *   - 肩部 (joints 0,1,2)：三轴交于一点 S
 *   - 肘部 (joint 3)：单旋转轴
 *   - 腕部 (joints 4,5,6)：三轴交于一点 W
 *
 * 冗余参数：肘部绕肩-腕轴线的旋转角 ψ (swivel angle / arm angle)。
 * 对每个 ψ 采样点，最多产生 8 组解析解（肩 2 × 肘 2 × 腕 2）。
 *
 * 参考：
 *   - Singh & Kreutz (1989): 原始 7R-SRS 闭式解推导
 *   - ssik (personalrobotics/ssik): Python 参考实现
 *   - MuJoCo openarm_bimanual.xml: 运动学参数来源
 *
 * 坐标系约定：
 *   - T_target 是末端连杆相对于基座的 4×4 齐次变换
 *   - 所有关节角单位为弧度
 *   - 关节限位与 config/openarm/axis.xml 一致
 */
#pragma once

#include <Eigen/Dense>
#include <vector>
#include <array>
#include <functional>
#include <string>

/**
 * @brief 单个 IK 解
 */
struct SRSIkSolution
{
    Eigen::Matrix<double, 7, 1> q;       // 7 个关节角 (rad)
    double fkResidual = 0.0;              // FK 闭合误差 (Frobenius 范数)
    bool inLimits = true;                 // 是否满足关节限位
    int branchId = -1;                    // 构型分支 ID (0-7)

    /// 按距参考构型的最大单关节偏差排序（越小越近）
    double distanceLInf(const Eigen::Matrix<double, 7, 1>& ref) const;
};

/**
 * @brief openArm SRS 解析逆运动学求解器
 *
 * 用法：
 * @code
 *   // 创建左臂求解器
 *   OpenArmSRSIk ik(OpenArmSRSIk::Side::LEFT);
 *
 *   // 求解所有满足限位的构型
 *   auto sols = ik.solve(T_target);
 *   if (!sols.empty()) {
 *       q_target = sols[0].q; // 最接近参考构型的解
 *   }
 *
 *   // 轨迹跟踪常用：从当前构型数值延续
 *   auto sols = ik.solve(T_target, q_current, 1);
 *   q_next = sols[0].q;
 * @endcode
 */
class OpenArmSRSIk
{
public:
    enum class Side { LEFT, RIGHT };

    static constexpr int DOF = 7;
    static constexpr int DEFAULT_SWIVEL_SAMPLES = 16;

    /**
     * @brief 用指定侧（左臂/右臂）构造求解器
     *
     * 左右臂的关节轴 Y 方向镜像：
     *   - 左臂 joint1 轴 [0, +1, 0], 右臂 joint1 轴 [0, -1, 0]
     *   - 连杆长度、Z 轴关节完全一致
     */
    explicit OpenArmSRSIk(Side side);

    // --- 运动学参数查询 ---

    /** @return 肩部枢轴点 (基座坐标系) */
    const Eigen::Vector3d& shoulderPivot() const { return m_shoulderPivot; }

    /** @return 腕部枢轴点 (基座坐标系, q=0 时) */
    const Eigen::Vector3d& wristPivot() const { return m_wristPivot; }

    /** @return 上臂长度 |S - E| (m) */
    double upperArmLength() const { return m_L_se; }

    /** @return 前臂长度 |E - W| (m) */
    double forearmLength() const { return m_L_ew; }

    /** @return 最大可达距离 (m) */
    double maxReach() const { return m_L_se + m_L_ew; }

    /** @return 各关节在 q=0 时的基座坐标原点 */
    const std::array<Eigen::Vector3d, DOF>& jointOrigins() const { return m_origins; }

    // --- 正运动学 ---

    /**
     * @brief POE 正运动学：关节角 -> 末端位姿
     * @param q 7 个关节角 (rad)
     * @return 4×4 齐次变换矩阵 (末端连杆相对于基座)
     */
    Eigen::Matrix4d forwardKinematics(const Eigen::Matrix<double, 7, 1>& q) const;

    // --- 逆运动学 ---

    /**
     * @brief SRS 解析逆运动学（完整枚举）
     *
     * 在肩-腕轴线的臂角 ψ ∈ [-π, π] 上均匀采样 swivelSamples 个点，
     * 对每个 ψ 生成最多 8 个候选构型（肩 × 肘 × 腕符号组合），
     * 经 FK 验证、关节限位过滤后返回。
     *
     * @param T_target  目标末端位姿 (4×4)
     * @param q_ref     参考关节构型，用于结果排序（越近越靠前）
     * @param maxSolutions  最多返回解的数量，0 = 全部
     * @param swivelSamples 臂角采样数，默认 16
     * @param respectLimits  是否过滤超出关节限位的解
     * @return 所有有效 IK 解，按距 q_ref 的 L∞ 距离升序
     */
    std::vector<SRSIkSolution> solve(
        const Eigen::Matrix4d& T_target,
        const Eigen::Matrix<double, 7, 1>& q_ref =
            Eigen::Matrix<double, 7, 1>::Zero(),
        int maxSolutions = 0,
        int swivelSamples = DEFAULT_SWIVEL_SAMPLES,
        bool respectLimits = true) const;

    /**
     * @brief 轨迹跟踪快速路径：从当前关节角数值延续到目标位姿
     *
     * 使用 Newton 法从 q_seed 出发跟踪到 T_target，避免完整解析求解。
     * 适用于平滑轨迹上的连续 IK 查询（~0.2ms vs ~5ms）。
     *
     * @param q_seed  当前关节角 (7×1)
     * @param T_target  下一目标位姿
     * @param maxIters  Newton 最大迭代次数
     * @param tolerance 收敛容差
     * @return 跟踪结果（失败时 q 为 NaN）
     */
    SRSIkSolution track(
        const Eigen::Matrix<double, 7, 1>& q_seed,
        const Eigen::Matrix4d& T_target,
        int maxIters = 15,
        double tolerance = 1e-6) const;

    /**
     * @brief 检查关节角是否位于限位内，并可要求额外安全余量
     * @param q 待检查的七轴关节角
     * @param margin 与上下限至少保留的距离，单位 rad
     */
    bool isWithinLimits(const Eigen::Matrix<double, 7, 1>& q,
                        double margin = 0.0) const;

    /**
     * @brief 返回所有关节到最近限位的最小距离
     *
     * 返回负值表示至少一个关节已经越界。该值用于 IK 分支评分，避免虽然
     * 数学上合法、但紧贴软限位的构型被轨迹规划器选中。
     */
    double minimumLimitMargin(const Eigen::Matrix<double, 7, 1>& q) const;

    /**
     * @brief 将关节角投影到限位闭区间内
     *
     * 仿真/反馈噪声可能让肘关节等贴边轴出现 1e-4 rad 量级越限。
     * MoveL 用实测关节做 IK 种子前应先投影，否则 track/solve 会直接失败。
     */
    Eigen::Matrix<double, 7, 1> clampToLimits(
        const Eigen::Matrix<double, 7, 1>& q) const;

    // --- Jacobian ---

    /**
     * @brief 解析几何 Jacobian (6×7)，基座坐标系
     *
     * 旋转关节 i: J_v = z_i × (p_n - p_i), J_ω = z_i
     */
    Eigen::Matrix<double, 6, DOF> jacobian(
        const Eigen::Matrix<double, 7, 1>& q) const;

private:
    Side m_side;

    // --- 运动学常量 (从 MuJoCo/ssik 提取) ---
    Eigen::Vector3d m_shoulderPivot;    // 肩部枢轴 S
    Eigen::Vector3d m_wristPivot;       // 腕部枢轴 W (q=0)
    double m_L_se;                      // 上臂长度 (m)
    double m_L_ew;                      // 前臂长度 (m)
    Eigen::Vector3d m_upperHome;        // 上臂方向向量 (q=0, 基座系)
    Eigen::Vector3d m_forearmHome;      // 前臂方向向量 (q=0, 基座系)
    Eigen::Vector3d m_eeOffsetLocal;    // 末端相对于腕枢轴的偏移 (基座系, q=0)

    // 关节轴（单位向量，关节自身坐标系）
    std::array<Eigen::Vector3d, DOF> m_axes;

    // 关节原点 (T_left 的平移部分，即从父连杆到关节的位移)
    std::array<Eigen::Vector3d, DOF> m_T_left_trans;

    // 关节在 q=0 时的基座坐标系原点（累积）
    std::array<Eigen::Vector3d, DOF> m_origins;

    // 关节限位 (rad)
    std::array<double, DOF> m_limitLower;
    std::array<double, DOF> m_limitUpper;

    // --- 内部辅助 ---

    /// 计算关节原点（累积变换，q=0）
    void computeOrigins();

    /// 两个非共线向量的最小旋转矩阵（u → v 方向）
    static Eigen::Matrix3d minRotation(const Eigen::Vector3d& u,
                                       const Eigen::Vector3d& v);

    /// 绕任意轴旋转 angle 弧度的旋转矩阵 (Rodrigues)
    static Eigen::Matrix3d axisAngleMatrix(const Eigen::Vector3d& axis,
                                           double angle);

    /// Subproblem 4: 解 h · (Rot(k, q) p) = delta，最多 2 解
    static std::vector<double> solveSP4(const Eigen::Vector3d& h,
                                        const Eigen::Vector3d& k,
                                        const Eigen::Vector3d& p,
                                        double delta);

    /// 任意三轴分解 (广义 Davenport): R = Rot(n0,q0)·Rot(n1,q1)·Rot(n2,q2)
    static std::vector<std::array<double, 3>> decompose3Axis(
        const Eigen::Matrix3d& R,
        const Eigen::Vector3d& n0,
        const Eigen::Vector3d& n1,
        const Eigen::Vector3d& n2);

    /// 构建 swivel 基向量 (垂直于 u_sw 的平面)
    static std::pair<Eigen::Vector3d, Eigen::Vector3d>
        swivelBasis(const Eigen::Vector3d& u_sw);

    /// 计算指定关节之前的累积变换 (R, p)
    void frameAtJoint(const Eigen::Matrix<double, 7, 1>& q,
                      int jointIdx,
                      Eigen::Matrix3d& R,
                      Eigen::Vector3d& p) const;
};

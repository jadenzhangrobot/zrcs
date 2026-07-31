/**
 * @file OpenArmRobot.cpp
 * @brief openArm 机器人模型实现
 */
#include "model/OpenArmRobot.h"

#include <algorithm>
#include <cmath>

namespace
{
// 绝对余量用于禁止贴边命令；首选余量用于在多个合法分支之间留出更充足的
// 后续 MoveL 空间。单位均为 rad。
constexpr double kRequiredLimitMargin = 0.02;
constexpr double kPreferredLimitMargin = 5.0 * M_PI / 180.0;
constexpr double kMaximumFkResidual = 1e-5;
constexpr int kAnalyticSwivelSamples = 64;
}

OpenArmRobot::OpenArmRobot(const std::string& name, OpenArmSRSIk::Side side)
    : RobotModel(name, "openarm_srs", 7)
    , m_ik(side)
{
    joints_.reserve(7);

    // OpenArmRobot 的一个实例只描述单臂。右臂的物理轴从 8 开始，左臂从
    // 0 开始；这必须与 model.xml/axis.xml 的映射保持一致。
    const int firstAxisId = (side == OpenArmSRSIk::Side::LEFT) ? 0 : 8;
    for (int i = 0; i < 7; i++)
    {
        ModelJoint j;
        j.axisId = firstAxisId + i;
        j.type   = JointType::REVOLUTE;
        j.offset = 0.0;
        joints_.push_back(j);
    }
}

bool OpenArmRobot::forwardKinematics(
    const Eigen::VectorXd& jointPos,
    Eigen::Matrix4d& toolPose) const
{
    if (jointPos.size() != 7)
        return false;

    Eigen::Matrix<double, 7, 1> q;
    for (int i = 0; i < 7; i++) q(i) = jointPos(i);

    toolPose = m_ik.forwardKinematics(q);
    return true;
}

bool OpenArmRobot::inverseKinematics(
    const Eigen::Matrix4d& toolPose,
    const Eigen::VectorXd& currentJointPos,
    Eigen::VectorXd& targetJointPos) const
{
    if (currentJointPos.size() != 7 || !toolPose.allFinite() ||
        !currentJointPos.allFinite())
        return false;

    Eigen::Matrix<double, 7, 1> q_meas;
    for (int i = 0; i < 7; i++) q_meas(i) = currentJointPos(i);

    // 实测/仿真反馈可能因接触、重力或数值噪声略微越出软限位
    // （openArm 肘关节下限恰为 0，home 附近尤易出现 j3=-1e-4 量级）。
    // 先把种子投影回限位，再做 IK；否则 track/solve 会因 inLimits=false
    // 或 margin 为负而把整段 MoveL 拒绝掉。
    const Eigen::Matrix<double, 7, 1> q_ref = m_ik.clampToLimits(q_meas);
    const double measuredMargin = m_ik.minimumLimitMargin(q_meas);
    const double currentMargin = m_ik.minimumLimitMargin(q_ref);
    if (!std::isfinite(currentMargin))
        return false;

    // 目标已是当前 FK 位姿时直接保持测量值，避免无意义的重求解。
    // 即使测量值有微小越限，保持原命令也比失败停机更安全。
    const Eigen::Matrix4d currentPose = m_ik.forwardKinematics(q_meas);
    if ((currentPose - toolPose).norm() <= kMaximumFkResidual)
    {
        targetJointPos = currentJointPos;
        return true;
    }

    // 当前已贴边/微越限时，允许从边界出发离开；离开贴边区后仍要求
    // 绝对安全余量，避免轨迹中途重新贴边。
    const double minAcceptMargin =
        (measuredMargin < kRequiredLimitMargin ||
         currentMargin < kRequiredLimitMargin)
            ? 0.0
            : kRequiredLimitMargin;

    // MoveL 每个周期的目标变化很小，优先从当前构型做数值延续可以保持分支
    // 连续且避免解析枚举的动态分配。跟踪结果必须同时通过限位与 FK 闭环验证。
    const auto tracked = m_ik.track(q_ref, toolPose);
    const bool trackedValid = tracked.q.allFinite() && tracked.inLimits &&
                              tracked.fkResidual <= kMaximumFkResidual;
    const double trackedMargin =
        trackedValid ? m_ik.minimumLimitMargin(tracked.q)
                     : -std::numeric_limits<double>::infinity();
    if (trackedValid && trackedMargin >= kPreferredLimitMargin)
    {
        targetJointPos = tracked.q;
        return true;
    }

    // 数值跟踪靠近限位或未收敛时，枚举更多 swivel 样本。先选满足首选
    // 余量且离当前构型最近的解；若不存在，则在可接受余量的解中选择
    // 限位余量最大、其次离当前构型最近的一个。
    const auto solutions = m_ik.solve(
        toolPose, q_ref, /*maxSolutions=*/0, kAnalyticSwivelSamples,
        /*respectLimits=*/true);

    const SRSIkSolution* bestPreferred = nullptr;
    const SRSIkSolution* bestFallback = nullptr;
    double bestPreferredDistance = std::numeric_limits<double>::infinity();
    double bestFallbackMargin = -std::numeric_limits<double>::infinity();
    double bestFallbackDistance = std::numeric_limits<double>::infinity();

    for (const auto& solution : solutions)
    {
        if (!solution.q.allFinite() || solution.fkResidual > kMaximumFkResidual)
            continue;

        const double margin = m_ik.minimumLimitMargin(solution.q);
        if (margin < minAcceptMargin)
            continue;

        const double distance = solution.distanceLInf(q_ref);
        if (margin >= kPreferredLimitMargin)
        {
            if (!bestPreferred || distance < bestPreferredDistance)
            {
                bestPreferred = &solution;
                bestPreferredDistance = distance;
            }
        }
        else if (!bestFallback || margin > bestFallbackMargin ||
                 (std::abs(margin - bestFallbackMargin) < 1e-12 &&
                  distance < bestFallbackDistance))
        {
            bestFallback = &solution;
            bestFallbackMargin = margin;
            bestFallbackDistance = distance;
        }
    }

    if (bestPreferred)
    {
        targetJointPos = bestPreferred->q;
        return true;
    }
    if (bestFallback)
    {
        targetJointPos = bestFallback->q;
        return true;
    }

    // 解析采样可能恰好错过一个有效 swivel 角，因此保留数值结果作为最后
    // 回退；仍拒绝越界或比当前种子更贴边的结果。
    if (!trackedValid || trackedMargin < minAcceptMargin)
        return false;

    targetJointPos = tracked.q;
    return true;
}

bool OpenArmRobot::jacobian(
    const Eigen::VectorXd& jointPos,
    Eigen::MatrixXd& J) const
{
    if (jointPos.size() != 7)
        return false;

    Eigen::Matrix<double, 7, 1> q;
    for (int i = 0; i < 7; i++) q(i) = jointPos(i);

    J = m_ik.jacobian(q);
    return true;
}

double OpenArmRobot::manipulability(const Eigen::VectorXd& jointPos) const
{
    Eigen::MatrixXd J;
    if (!jacobian(jointPos, J))
        return 0.0;

    Eigen::MatrixXd JJt = J * J.transpose();
    return std::sqrt(std::abs(JJt.determinant()));
}

std::vector<SRSIkSolution> OpenArmRobot::solveAll(
    const Eigen::Matrix4d& toolPose,
    int swivelSamples) const
{
    Eigen::Matrix<double, 7, 1> q_zero = Eigen::Matrix<double, 7, 1>::Zero();
    return m_ik.solve(toolPose, q_zero, /*maxSolutions=*/0, swivelSamples);
}

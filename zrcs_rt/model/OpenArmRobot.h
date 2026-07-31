/**
 * @file OpenArmRobot.h
 * @brief openArm v2.0 双臂机器人模型
 *
 * 封装 SRS 解析逆运动学求解器到 RobotModel 框架。
 * 每臂 7-DOF (不含手爪)，使用 POE 正运动学 + Singh-Kreutz 解析 IK。
 *
 * 左右臂共享连杆参数，仅 Y 轴关节方向镜像。
 */
#pragma once

#include "model/RobotModel.h"
#include "model/OpenArmSRSIk.h"

class OpenArmRobot : public RobotModel
{
public:
    /**
     * @brief 构造 openArm 单臂模型
     * @param name 模型名称 (如 "openarm_left", "openarm_right")
     * @param side 左臂/右臂
     *
     * 连杆参数自动从 MuJoCo/ssik 提取的常量加载。
     */
    OpenArmRobot(const std::string& name, OpenArmSRSIk::Side side);

    // --- RobotModel 接口 ---

    bool forwardKinematics(
        const Eigen::VectorXd& jointPos,
        Eigen::Matrix4d& toolPose) const override;

    bool inverseKinematics(
        const Eigen::Matrix4d& toolPose,
        const Eigen::VectorXd& currentJointPos,
        Eigen::VectorXd& targetJointPos) const override;

    bool jacobian(
        const Eigen::VectorXd& jointPos,
        Eigen::MatrixXd& J) const override;

    double manipulability(const Eigen::VectorXd& jointPos) const override;

    // --- 扩展接口 ---

    /** @return 底层 SRS 求解器的只读引用 */
    const OpenArmSRSIk& srsIk() const { return m_ik; }

    /**
     * @brief 获取所有 IK 分支（完整枚举）
     *
     * 比标准 IK 接口返回更多解，用于路径规划中的构型选择。
     */
    std::vector<SRSIkSolution> solveAll(
        const Eigen::Matrix4d& toolPose,
        int swivelSamples = 16) const;

private:
    OpenArmSRSIk m_ik;
};


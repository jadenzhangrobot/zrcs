/**
 * @file CartesianRobot.h
 * @brief 笛卡尔/龙门架机器人运动学模型
 *
 * XYZ直角坐标系机器人，关节位置直接映射为笛卡尔坐标。
 * 支持 3轴(XYZ)、4轴(XYZ+C)、5轴(XYZ+AC) 等配置。
 * 每个关节通过 ModelJoint::axis 指定映射到 X/Y/Z 中的哪个轴。
 */
#pragma once

#include "RobotModel.h"

class CartesianRobot : public RobotModel
{
public:
    CartesianRobot(const std::string& name, int dof,
                   const std::vector<ModelJoint>& joints,
                   const Eigen::Matrix4d& baseTf = Eigen::Matrix4d::Identity(),
                   const Eigen::Matrix4d& toolTf = Eigen::Matrix4d::Identity())
        : RobotModel(name, "cartesian", dof)
    {
        joints_ = joints;
        baseTf_ = baseTf;
        toolTf_ = toolTf;
    }

    /**
     * @brief 正运动学
     *
     * 每个移动关节直接映射到对应的笛卡尔轴位移，
     * 旋转关节映射为绕对应轴的旋转。
     */
    bool forwardKinematics(
        const Eigen::VectorXd& jointPos,
        Eigen::Matrix4d& toolPose) const override;

    /**
     * @brief 逆运动学
     *
     * 笛卡尔机器人的IK是解析的：
     * 从目标位姿中提取对应轴的分量即可。
     */
    bool inverseKinematics(
        const Eigen::Matrix4d& toolPose,
        const Eigen::VectorXd& currentJointPos,
        Eigen::VectorXd& targetJointPos) const override;

    /**
     * @brief 解析 Jacobian
     *
     * 笛卡尔机器人的 Jacobian 非常简单:
     * 移动关节: 对应轴方向为1，其余为0
     * 旋转关节: 对应轴方向角速度为1，其余为0
     */
    bool jacobian(
        const Eigen::VectorXd& jointPos,
        Eigen::MatrixXd& J) const override;
};


/**
 * @file parallelRobot.h
 * @brief Delta并联机器人运动学模型
 *
 * 实现标准3-DOF Delta并联机器人的正逆运动学。
 * 三条对称支链，固定平台和运动平台均为等边三角形分布。
 *
 * 参考: R.L. Williams, "The Delta Parallel Robot: Kinematics Solutions"
 */
#pragma once

#include "RobotModel.h"

class DeltaRobot : public RobotModel
{
private:
    double sb_;    // 固定平台等效半径 (基座三角形外接圆半径)
    double sp_;    // 运动平台等效半径
    double L_;     // 主动臂长度 (上臂)
    double l_;     // 从动臂长度 (下臂/平行四边形连杆)

public:
    DeltaRobot(const std::string& name,
               const std::vector<ModelJoint>& joints,
               double basePlatformRadius,
               double mobilePlatformRadius,
               double upperArmLength,
               double lowerArmLength,
               const Eigen::Matrix4d& baseTf = Eigen::Matrix4d::Identity(),
               const Eigen::Matrix4d& toolTf = Eigen::Matrix4d::Identity())
        : RobotModel(name, "delta", 3),
          sb_(basePlatformRadius),
          sp_(mobilePlatformRadius),
          L_(upperArmLength),
          l_(lowerArmLength)
    {
        joints_ = joints;
        baseTf_ = baseTf;
        toolTf_ = toolTf;
    }

    /**
     * @brief 逆运动学 (解析解)
     *
     * 给定末端平台位置 (x, y, z)，求三个主动臂角度。
     * 每条支链独立求解。
     *
     * 坐标系: 固定平台中心为原点, z轴向上
     */
    bool inverseKinematics(
        const Eigen::Matrix4d& toolPose,
        const Eigen::VectorXd& currentJointPos,
        Eigen::VectorXd& targetJointPos) const override;

    /**
     * @brief 正运动学 (解析解)
     *
     * 给定三个主动臂角度，求末端平台位置。
     * 方法: 三球交汇法 — 三个球面的交点。
     */
    bool forwardKinematics(
        const Eigen::VectorXd& jointPos,
        Eigen::Matrix4d& toolPose) const override;
};


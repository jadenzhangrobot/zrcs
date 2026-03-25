/**
 * @file cartesianRobot.h
 * @brief 笛卡尔/龙门架机器人运动学模型
 *
 * XYZ直角坐标系机器人，关节位置直接映射为笛卡尔坐标。
 * 支持 3轴(XYZ)、4轴(XYZ+C)、5轴(XYZ+AC) 等配置。
 * 每个关节通过 ModelJoint::axis 指定映射到 X/Y/Z 中的哪个轴。
 */
#ifndef CARTESIAN_ROBOT_H
#define CARTESIAN_ROBOT_H

#include "robotModel.h"

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
        Eigen::Matrix4d& toolPose) const override
    {
        if (jointPos.size() != dof_)
        {
            return false;
        }

        Eigen::Matrix4d T = Eigen::Matrix4d::Identity();

        for (int i = 0; i < dof_; i++)
        {
            const auto& j = joints_[i];
            double q = jointPos(i) + j.offset;

            if (j.type == JointType::PRISMATIC)
            {
                // 移动关节: 平移
                switch (j.axis)
                {
                    case 'X': case 'x': T(0, 3) += q; break;
                    case 'Y': case 'y': T(1, 3) += q; break;
                    case 'Z': case 'z': T(2, 3) += q; break;
                }
            }
            else // REVOLUTE
            {
                // 旋转关节: 绕轴旋转
                Eigen::Matrix3d R = Eigen::Matrix3d::Identity();
                switch (j.axis)
                {
                    case 'X': case 'x':
                        R = Eigen::AngleAxisd(q, Eigen::Vector3d::UnitX()).toRotationMatrix();
                        break;
                    case 'Y': case 'y':
                        R = Eigen::AngleAxisd(q, Eigen::Vector3d::UnitY()).toRotationMatrix();
                        break;
                    case 'Z': case 'z':
                        R = Eigen::AngleAxisd(q, Eigen::Vector3d::UnitZ()).toRotationMatrix();
                        break;
                }
                // 旋转应用到当前变换
                Eigen::Matrix4d Rmat = Eigen::Matrix4d::Identity();
                Rmat.block<3,3>(0,0) = R;
                T = T * Rmat;
            }
        }

        toolPose = baseTf_ * T * toolTf_;
        return true;
    }

    /**
     * @brief 逆运动学
     *
     * 笛卡尔机器人的IK是解析的：
     * 从目标位姿中提取对应轴的分量即可。
     */
    bool inverseKinematics(
        const Eigen::Matrix4d& toolPose,
        const Eigen::VectorXd& currentJointPos,
        Eigen::VectorXd& targetJointPos) const override
    {
        if (currentJointPos.size() != dof_)
        {
            return false;
        }

        // 剥离 baseTf 和 toolTf
        Eigen::Matrix4d internalPose = baseTf_.inverse() * toolPose * toolTf_.inverse();

        targetJointPos.resize(dof_);

        for (int i = 0; i < dof_; i++)
        {
            const auto& j = joints_[i];

            if (j.type == JointType::PRISMATIC)
            {
                switch (j.axis)
                {
                    case 'X': case 'x': targetJointPos(i) = internalPose(0, 3) - j.offset; break;
                    case 'Y': case 'y': targetJointPos(i) = internalPose(1, 3) - j.offset; break;
                    case 'Z': case 'z': targetJointPos(i) = internalPose(2, 3) - j.offset; break;
                    default: targetJointPos(i) = 0; break;
                }
            }
            else // REVOLUTE — 从旋转矩阵提取角度
            {
                Eigen::Matrix3d R = internalPose.block<3,3>(0,0);
                Eigen::Vector3d euler = R.eulerAngles(2, 1, 0); // ZYX顺序

                switch (j.axis)
                {
                    case 'X': case 'x': targetJointPos(i) = euler(2) - j.offset; break;
                    case 'Y': case 'y': targetJointPos(i) = euler(1) - j.offset; break;
                    case 'Z': case 'z': targetJointPos(i) = euler(0) - j.offset; break;
                    default: targetJointPos(i) = 0; break;
                }
            }
        }

        return true;
    }

    /**
     * @brief 解析 Jacobian
     *
     * 笛卡尔机器人的 Jacobian 非常简单:
     * 移动关节: 对应轴方向为1，其余为0
     * 旋转关节: 对应轴方向角速度为1，其余为0
     */
    bool jacobian(
        const Eigen::VectorXd& jointPos,
        Eigen::MatrixXd& J) const override
    {
        if (jointPos.size() != dof_)
        {
            return false;
        }

        J = Eigen::MatrixXd::Zero(6, dof_);

        for (int i = 0; i < dof_; i++)
        {
            const auto& j = joints_[i];
            int axisIdx = 0;
            switch (j.axis)
            {
                case 'X': case 'x': axisIdx = 0; break;
                case 'Y': case 'y': axisIdx = 1; break;
                case 'Z': case 'z': axisIdx = 2; break;
            }

            if (j.type == JointType::PRISMATIC)
            {
                J(axisIdx, i) = 1.0;  // 线速度
            }
            else // REVOLUTE
            {
                J(3 + axisIdx, i) = 1.0;  // 角速度
            }
        }

        return true;
    }
};

#endif // CARTESIAN_ROBOT_H

/**
 * @file cartesianRobot.cpp
 * @brief 笛卡尔/龙门架机器人运动学模型实现
 */
#include "model/CartesianRobot.h"

bool CartesianRobot::forwardKinematics(
    const Eigen::VectorXd& jointPos,
    Eigen::Matrix4d& toolPose) const
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

bool CartesianRobot::inverseKinematics(
    const Eigen::Matrix4d& toolPose,
    const Eigen::VectorXd& currentJointPos,
    Eigen::VectorXd& targetJointPos) const
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
            Eigen::Vector3d euler = R.canonicalEulerAngles(2, 1, 0); // ZYX顺序

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

bool CartesianRobot::jacobian(
    const Eigen::VectorXd& jointPos,
    Eigen::MatrixXd& J) const
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

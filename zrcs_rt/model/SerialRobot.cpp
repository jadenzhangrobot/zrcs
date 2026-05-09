/**
 * @file SerialRobot.cpp
 * @brief 串联机械臂运动学模型实现
 */
#include "model/SerialRobot.h"
#include <cmath>

Eigen::Matrix4d SerialRobot::dhTransform(double a, double alpha, double d, double theta)
{
    double ct = std::cos(theta);
    double st = std::sin(theta);
    double ca = std::cos(alpha);
    double sa = std::sin(alpha);

    Eigen::Matrix4d T;
    T << ct, -st * ca,  st * sa, a * ct,
         st,  ct * ca, -ct * sa, a * st,
         0,   sa,       ca,      d,
         0,   0,        0,       1;
    return T;
}

bool SerialRobot::forwardKinematics(
    const Eigen::VectorXd& jointPos,
    Eigen::Matrix4d& toolPose) const
{
    if (jointPos.size() != dof_)
    {
        return false;
    }

    Eigen::Matrix4d T = baseTf_;

    for (int i = 0; i < dof_; i++)
    {
        const auto& j = joints_[i];
        double q = jointPos(i) + j.offset;

        if (j.type == JointType::REVOLUTE)
        {
            T = T * dhTransform(j.dh_a, j.dh_alpha, j.dh_d, j.dh_theta + q);
        }
        else // PRISMATIC
        {
            T = T * dhTransform(j.dh_a, j.dh_alpha, j.dh_d + q, j.dh_theta);
        }
    }

    toolPose = T * toolTf_;
    return true;
}

bool SerialRobot::inverseKinematics(
    const Eigen::Matrix4d& toolPose,
    const Eigen::VectorXd& currentJointPos,
    Eigen::VectorXd& targetJointPos) const
{
    if (currentJointPos.size() != dof_)
    {
        return false;
    }

    constexpr int maxIter = 100;
    constexpr double tolerance = 1e-6;
    constexpr double lambda = 0.01;        // 阻尼系数
    constexpr double stepScale = 1.0;

    Eigen::VectorXd q = currentJointPos;

    for (int iter = 0; iter < maxIter; iter++)
    {
        Eigen::Matrix4d currentPose;
        if (!forwardKinematics(q, currentPose))
        {
            return false;
        }

        // 计算位姿误差 (6x1: 3线性 + 3角度)
        Eigen::Vector3d posError = toolPose.block<3,1>(0,3) - currentPose.block<3,1>(0,3);

        // 角度误差: 从旋转矩阵差提取
        Eigen::Matrix3d Rd = toolPose.block<3,3>(0,0);
        Eigen::Matrix3d Rc = currentPose.block<3,3>(0,0);
        Eigen::Matrix3d Re = Rd * Rc.transpose();

        // 从旋转矩阵提取轴角表示的角度误差
        Eigen::AngleAxisd aa(Re);
        Eigen::Vector3d oriError = aa.angle() * aa.axis();

        Eigen::Matrix<double, 6, 1> error;
        error.head<3>() = posError;
        error.tail<3>() = oriError;

        // 检查收敛
        if (error.norm() < tolerance)
        {
            targetJointPos = q;
            return true;
        }

        // 计算 Jacobian
        Eigen::MatrixXd J;
        if (!jacobian(q, J))
        {
            return false;
        }

        // DLS: dq = J^T * (J * J^T + lambda^2 * I)^{-1} * error
        Eigen::MatrixXd JJt = J * J.transpose();
        Eigen::MatrixXd dampedInv = JJt + lambda * lambda * Eigen::MatrixXd::Identity(6, 6);
        Eigen::VectorXd dq = J.transpose() * dampedInv.ldlt().solve(error);

        q += stepScale * dq;
    }

    // 未收敛，返回最后结果
    targetJointPos = q;
    return false;
}

bool SerialRobot::jacobian(
    const Eigen::VectorXd& jointPos,
    Eigen::MatrixXd& J) const
{
    if (jointPos.size() != dof_)
    {
        return false;
    }

    J.resize(6, dof_);

    // 计算各关节的累积变换
    std::vector<Eigen::Matrix4d> transforms(dof_ + 1);
    transforms[0] = baseTf_;

    for (int i = 0; i < dof_; i++)
    {
        const auto& j = joints_[i];
        double q = jointPos(i) + j.offset;

        if (j.type == JointType::REVOLUTE)
        {
            transforms[i + 1] = transforms[i] * dhTransform(j.dh_a, j.dh_alpha, j.dh_d, j.dh_theta + q);
        }
        else
        {
            transforms[i + 1] = transforms[i] * dhTransform(j.dh_a, j.dh_alpha, j.dh_d + q, j.dh_theta);
        }
    }

    // 末端位置（含 toolTf）
    Eigen::Matrix4d Tn = transforms[dof_] * toolTf_;
    Eigen::Vector3d pn = Tn.block<3,1>(0,3);

    for (int i = 0; i < dof_; i++)
    {
        Eigen::Vector3d zi = transforms[i].block<3,1>(0,2);  // z轴方向
        Eigen::Vector3d pi = transforms[i].block<3,1>(0,3);  // 关节位置

        if (joints_[i].type == JointType::REVOLUTE)
        {
            J.block<3,1>(0, i) = zi.cross(pn - pi);  // 线速度
            J.block<3,1>(3, i) = zi;                   // 角速度
        }
        else // PRISMATIC
        {
            J.block<3,1>(0, i) = zi;                   // 线速度
            J.block<3,1>(3, i) = Eigen::Vector3d::Zero(); // 无角速度
        }
    }

    return true;
}

double SerialRobot::manipulability(const Eigen::VectorXd& jointPos) const
{
    Eigen::MatrixXd J;
    if (!jacobian(jointPos, J))
    {
        return 0.0;
    }
    Eigen::MatrixXd JJt = J * J.transpose();
    return std::sqrt(std::abs(JJt.determinant()));
}

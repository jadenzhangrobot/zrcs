/**
 * @file RobotModel.cpp
 * @brief 运动学模型基类实现
 */
#include "model/RobotModel.h"

bool RobotModel::jacobian(
    const Eigen::VectorXd& jointPos,
    Eigen::MatrixXd& J) const
{
    const double delta = 1e-6;
    J.resize(6, dof_);

    Eigen::Matrix4d T0;
    if (!forwardKinematics(jointPos, T0))
    {
        return false;
    }

    Eigen::Vector3d p0 = T0.block<3,1>(0,3);
    Eigen::Matrix3d R0 = T0.block<3,3>(0,0);

    for (int i = 0; i < dof_; i++)
    {
        Eigen::VectorXd qd = jointPos;
        qd(i) += delta;

        Eigen::Matrix4d Td;
        if (!forwardKinematics(qd, Td))
        {
            return false;
        }

        // 线速度部分: dp/dq
        Eigen::Vector3d dp = (Td.block<3,1>(0,3) - p0) / delta;
        J.block<3,1>(0, i) = dp;

        // 角速度部分: 从旋转矩阵差分提取
        Eigen::Matrix3d dR = (Td.block<3,3>(0,0) - R0) / delta;
        Eigen::Matrix3d skew = dR * R0.transpose();
        J(3, i) = skew(2, 1);  // wx
        J(4, i) = skew(0, 2);  // wy
        J(5, i) = skew(1, 0);  // wz
    }
    return true;
}

double RobotModel::manipulability(const Eigen::VectorXd& jointPos) const
{
    Eigen::MatrixXd J;
    if (!jacobian(jointPos, J))
    {
        return 0.0;
    }
    Eigen::MatrixXd JJt = J * J.transpose();
    return std::sqrt(std::abs(JJt.determinant()));
}

const std::vector<int>& RobotModel::getAxisIds() const
{
    if (axisIdsCache_.empty())
    {
        axisIdsCache_.reserve(joints_.size());
        for (const auto& j : joints_)
        {
            axisIdsCache_.push_back(j.axisId);
        }
    }
    return axisIdsCache_;
}

Eigen::Matrix4d RobotModel::poseFromXYZRPY(double x, double y, double z,
                                             double rx, double ry, double rz)
{
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
    T.block<3,1>(0,3) = Eigen::Vector3d(x, y, z);

    Eigen::Matrix3d R;
    R = Eigen::AngleAxisd(rz, Eigen::Vector3d::UnitZ())
      * Eigen::AngleAxisd(ry, Eigen::Vector3d::UnitY())
      * Eigen::AngleAxisd(rx, Eigen::Vector3d::UnitX());
    T.block<3,3>(0,0) = R;
    return T;
}

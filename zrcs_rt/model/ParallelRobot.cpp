/**
 * @file parallelRobot.cpp
 * @brief Delta并联机器人运动学模型实现
 */
#include "model/ParallelRobot.h"
#include <cmath>

bool DeltaRobot::inverseKinematics(
    const Eigen::Matrix4d& toolPose,
    const Eigen::VectorXd& currentJointPos,
    Eigen::VectorXd& targetJointPos) const
{
    // 剥离坐标系偏移
    Eigen::Matrix4d internalPose = baseTf_.inverse() * toolPose * toolTf_.inverse();
    double x = internalPose(0, 3);
    double y = internalPose(1, 3);
    double z = internalPose(2, 3);

    targetJointPos.resize(3);

    // 三条支链均匀分布在120度间隔
    double angles[3] = {0.0, 2.0 * M_PI / 3.0, 4.0 * M_PI / 3.0};

    for (int i = 0; i < 3; i++)
    {
        double cos_a = std::cos(angles[i]);
        double sin_a = std::sin(angles[i]);

        // 将末端位置旋转到当前支链的对称平面
        double xi = x * cos_a + y * sin_a;
        double yi = -x * sin_a + y * cos_a;
        double zi = z;

        // 有效长度
        double a = sb_ - sp_ - xi;

        // 求解主动臂角度 theta_i
        // 几何约束: (a - L*cos(theta))^2 + yi^2 + (zi + L*sin(theta))^2 = l^2
        // 展开后变为: A*cos(theta) + B*sin(theta) + C = 0
        double A = 2.0 * L_ * a;
        double B = 2.0 * L_ * zi;
        double C = a * a + yi * yi + zi * zi + L_ * L_ - l_ * l_;

        // 解方程: A*cos + B*sin = -C
        double D = A * A + B * B;
        if (D < 1e-10)
        {
            return false;  // 退化情况
        }

        double disc = D - C * C;
        if (disc < 0)
        {
            return false;  // 无解，超出工作空间
        }

        // 取两个解中z值较大的（通常为上方解）
        double theta = std::atan2(B, A) - std::atan2(std::sqrt(disc), -C);

        targetJointPos(i) = theta - joints_[i].offset;
    }

    return true;
}

bool DeltaRobot::forwardKinematics(
    const Eigen::VectorXd& jointPos,
    Eigen::Matrix4d& toolPose) const
{
    if (jointPos.size() != 3)
    {
        return false;
    }

    // 三条支链的主动臂末端点 (肘部点)
    double angles[3] = {0.0, 2.0 * M_PI / 3.0, 4.0 * M_PI / 3.0};
    Eigen::Vector3d elbow[3];

    for (int i = 0; i < 3; i++)
    {
        double theta = jointPos(i) + joints_[i].offset;
        double cos_a = std::cos(angles[i]);
        double sin_a = std::sin(angles[i]);

        // 固定平台关节位置
        double bx = sb_ * cos_a;
        double by = sb_ * sin_a;

        // 主动臂末端位置 (在支链对称平面内)
        double ex = (sb_ - L_ * std::cos(theta)) * cos_a;
        double ey = (sb_ - L_ * std::cos(theta)) * sin_a;
        double ez = -L_ * std::sin(theta);

        elbow[i] = Eigen::Vector3d(ex, ey, ez);
    }

    // 运动平台中心相对于各肘部点的距离等于从动臂长度 l_
    // 但还要减去运动平台半径 sp_ 的偏移
    // 三球交汇求解
    // 球心为 elbow[i] 沿径向内缩 sp_
    Eigen::Vector3d center[3];
    for (int i = 0; i < 3; i++)
    {
        double cos_a = std::cos(angles[i]);
        double sin_a = std::sin(angles[i]);
        center[i] = Eigen::Vector3d(
            elbow[i](0) - sp_ * cos_a,
            elbow[i](1) - sp_ * sin_a,
            elbow[i](2));
    }

    // 求三个球面交点 |P - center[i]| = l_
    // 转化为线性方程组 + 一元二次方程
    // (x-x1)^2+(y-y1)^2+(z-z1)^2 = l^2
    // 两两相减得到两个线性方程，再代入一个球方程

    double x1 = center[0](0), y1 = center[0](1), z1 = center[0](2);
    double x2 = center[1](0), y2 = center[1](1), z2 = center[1](2);
    double x3 = center[2](0), y3 = center[2](1), z3 = center[2](2);

    // 方程 (2) - (1): 2*(x2-x1)*x + 2*(y2-y1)*y + 2*(z2-z1)*z = (x2^2+y2^2+z2^2) - (x1^2+y1^2+z1^2)
    double a11 = 2.0 * (x2 - x1), a12 = 2.0 * (y2 - y1), a13 = 2.0 * (z2 - z1);
    double b1 = (x2*x2 + y2*y2 + z2*z2) - (x1*x1 + y1*y1 + z1*z1);

    // 方程 (3) - (1):
    double a21 = 2.0 * (x3 - x1), a22 = 2.0 * (y3 - y1), a23 = 2.0 * (z3 - z1);
    double b2 = (x3*x3 + y3*y3 + z3*z3) - (x1*x1 + y1*y1 + z1*z1);

    // 用 z 表示 x 和 y (假设 a11*a22 - a21*a12 != 0)
    double det = a11 * a22 - a21 * a12;
    if (std::abs(det) < 1e-10)
    {
        return false;
    }

    // x = (c1 + c2*z), y = (d1 + d2*z)
    double c1 = (b1 * a22 - b2 * a12) / det;
    double c2 = (a13 * a22 - a23 * a12) / (-det);
    double d1 = (a11 * b2 - a21 * b1) / det;
    double d2 = (a11 * a23 - a21 * a13) / (-det);

    // 代入球方程 (1): (c1+c2*z-x1)^2 + (d1+d2*z-y1)^2 + (z-z1)^2 = l^2
    double ex1 = c1 - x1, ey1 = d1 - y1;
    double A_coeff = c2*c2 + d2*d2 + 1.0;
    double B_coeff = 2.0 * (ex1*c2 + ey1*d2 - z1);
    double C_coeff = ex1*ex1 + ey1*ey1 + z1*z1 - l_*l_;

    double disc = B_coeff*B_coeff - 4.0*A_coeff*C_coeff;
    if (disc < 0)
    {
        return false;
    }

    // 取z值较小的解（下方解，通常是工作空间内的解）
    double z_sol = (-B_coeff - std::sqrt(disc)) / (2.0 * A_coeff);
    double x_sol = c1 + c2 * z_sol;
    double y_sol = d1 + d2 * z_sol;

    toolPose = baseTf_;
    toolPose(0, 3) += x_sol;
    toolPose(1, 3) += y_sol;
    toolPose(2, 3) += z_sol;
    // Delta机器人运动平台不旋转，保持与固定平台平行
    toolPose = toolPose * toolTf_;

    return true;
}

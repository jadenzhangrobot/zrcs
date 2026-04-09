/**
 * @file serialRobot.h
 * @brief 串联机械臂运动学模型
 *
 * 基于标准DH参数的通用串联机器人运动学。
 * 适用于: 6轴机械臂(UR, KUKA, ABB), 4轴SCARA, 3轴码垛机等。
 * 通过不同的DH参数配置适配不同型号。
 *
 * FK: 逐级DH变换矩阵相乘
 * IK: 阻尼最小二乘(DLS)数值迭代，通用解适用于任意DOF
 * Jacobian: 解析几何法
 */
#pragma once

#include "RobotModel.h"

class SerialRobot : public RobotModel
{
public:
    SerialRobot(const std::string& name, int dof,
                const std::vector<ModelJoint>& joints,
                const Eigen::Matrix4d& baseTf = Eigen::Matrix4d::Identity(),
                const Eigen::Matrix4d& toolTf = Eigen::Matrix4d::Identity())
        : RobotModel(name, "serial", dof)
    {
        joints_ = joints;
        baseTf_ = baseTf;
        toolTf_ = toolTf;
    }

    /**
     * @brief 标准DH变换矩阵
     * T = Rot_z(theta) * Trans_z(d) * Trans_x(a) * Rot_x(alpha)
     */
    static Eigen::Matrix4d dhTransform(double a, double alpha, double d, double theta);

    /**
     * @brief 正运动学
     *
     * toolPose = baseTf * T_0 * T_1 * ... * T_(n-1) * toolTf
     * 每个 T_i = DH(a_i, alpha_i, d_i, theta_i + q_i)  (旋转关节)
     *          = DH(a_i, alpha_i, d_i + q_i, theta_i)  (移动关节)
     */
    bool forwardKinematics(
        const Eigen::VectorXd& jointPos,
        Eigen::Matrix4d& toolPose) const override;

    /**
     * @brief 逆运动学 — 阻尼最小二乘法 (Damped Least Squares / Levenberg-Marquardt)
     *
     * 通用数值迭代求解，适用于任意DOF串联结构。
     * 算法: q_new = q_old + J^T * (J * J^T + lambda^2 * I)^{-1} * e
     *
     * 从目标位姿中剥离 baseTf 和 toolTf:
     * 内部目标 = baseTf.inv * targetPose * toolTf.inv
     */
    bool inverseKinematics(
        const Eigen::Matrix4d& toolPose,
        const Eigen::VectorXd& currentJointPos,
        Eigen::VectorXd& targetJointPos) const override;

    /**
     * @brief 解析几何Jacobian
     *
     * 对于每个旋转关节 i:
     *   J_v_i = z_{i-1} × (p_n - p_{i-1})   (线速度列)
     *   J_w_i = z_{i-1}                       (角速度列)
     *
     * 对于每个移动关节 i:
     *   J_v_i = z_{i-1}
     *   J_w_i = 0
     */
    bool jacobian(
        const Eigen::VectorXd& jointPos,
        Eigen::MatrixXd& J) const override;

    /**
     * @brief 可操作度
     * w = sqrt(det(J * J^T))
     */
    double manipulability(const Eigen::VectorXd& jointPos) const override;
};


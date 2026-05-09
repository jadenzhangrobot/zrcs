/**
 * @file RobotModel.h
 * @brief 运动学模型基类，描述多轴之间的几何关系
 *
 * 模型层只关心坐标变换（FK/IK/Jacobian），不涉及：
 * - 关节硬件参数（maxVel/maxAcc/限位等在 Axis 层）
 * - 轨迹规划（在 ModeInterface/CmdNode 层）
 * - 笛卡尔限速（通过 Jacobian 映射关节限制动态计算）
 */
#pragma once

#include <Eigen/Dense>
#include <vector>
#include <string>

/**
 * @brief 关节类型
 */
enum class JointType
{
    REVOLUTE,     // 旋转关节
    PRISMATIC     // 移动关节
};

/**
 * @brief 模型层的关节描述
 *
 * 只包含几何信息，不重复 Axis 层的硬件参数。
 * 通过 axisId 引用 Controller::axes_ 中的物理轴。
 */
struct ModelJoint
{
    int axisId = 0;           // 对应的物理轴ID
    JointType type = JointType::REVOLUTE;
    double offset = 0;        // 零位偏移 (模型坐标系 vs 轴坐标系)

    // DH参数 (标准DH, 串联机器人专用)
    double dh_a     = 0;      // 连杆长度
    double dh_alpha = 0;      // 连杆扭角
    double dh_d     = 0;      // 连杆偏距
    double dh_theta = 0;      // 关节角偏移

    // 笛卡尔轴映射 (笛卡尔机器人专用)
    char axis = 'X';          // 映射到哪个笛卡尔轴: X, Y, Z
};

/**
 * @brief 运动学模型基类
 *
 * 设计原则：
 * 1. FK/IK/Jacobian 为 const 方法（无内部状态），参考 Pinocchio Model/Data 分离
 * 2. 支持 baseFrame（基坐标系偏移）和 toolFrame（TCP偏移），参考 KDL/MoveIt
 * 3. 不强制 FK/IK — 不是所有设备都需要运动学，默认返回 false
 * 4. 通过 axisId 引用物理轴，不重复定义硬件参数
 */
class RobotModel
{
protected:
    std::string name_;
    std::string type_;          // "serial", "delta", "cartesian"
    int dof_;                   // 自由度数
    std::vector<ModelJoint> joints_;
    Eigen::Matrix4d baseTf_  = Eigen::Matrix4d::Identity();  // 基坐标系偏移
    Eigen::Matrix4d toolTf_  = Eigen::Matrix4d::Identity();  // 工具坐标系(TCP)偏移
    double payloadMass_ = 0;  // 末端负载质量(kg)

    // 缓存: 预计算并在 NRT 中填充，RT 路径零分配读取
    mutable std::vector<int> axisIdsCache_{};

public:
    RobotModel(const std::string& name, const std::string& type, int dof)
        : name_(name), type_(type), dof_(dof)
    {
        joints_.reserve(dof);
    }

    virtual ~RobotModel() = default;

    /**
     * @brief 正运动学: 关节位置 → 末端位姿
     * @param jointPos 关节位置向量 (size = dof)
     * @param toolPose 输出的 4x4 齐次变换矩阵 (含 baseTf 和 toolTf)
     * @return true 求解成功
     */
    virtual bool forwardKinematics(
        const Eigen::VectorXd& jointPos,
        Eigen::Matrix4d& toolPose) const
    {
        return false;
    }

    /**
     * @brief 逆运动学: 末端位姿 → 关节位置
     * @param toolPose 目标 4x4 齐次变换矩阵
     * @param currentJointPos 当前关节位置（作为初始猜测值）
     * @param targetJointPos 输出的目标关节位置
     * @return true 求解成功
     */
    virtual bool inverseKinematics(
        const Eigen::Matrix4d& toolPose,
        const Eigen::VectorXd& currentJointPos,
        Eigen::VectorXd& targetJointPos) const
    {
        return false;
    }

    /**
     * @brief 计算 Jacobian 矩阵 (6 x dof)
     *
     * 默认实现使用数值微分（前向差分），子类可覆写为解析形式。
     * Jacobian 将关节速度映射到笛卡尔空间速度:
     *   v_cartesian = J(q) * dq
     *
     * @param jointPos 当前关节位置
     * @param J 输出的 6×dof Jacobian矩阵 (前3行:线速度, 后3行:角速度)
     */
    virtual bool jacobian(
        const Eigen::VectorXd& jointPos,
        Eigen::MatrixXd& J) const;

    /**
     * @brief 可操作度（manipulability）
     *
     * 衡量当前构型下机器人的运动灵活性。
     * 计算公式: w = sqrt(det(J * J^T))
     *
     * @return 可操作度值，越大越灵活，接近0表示接近奇异
     */
    virtual double manipulability(const Eigen::VectorXd& jointPos) const;

    /**
     * @brief 检测是否接近奇异构型
     */
    bool isNearSingularity(const Eigen::VectorXd& jointPos,
                            double threshold = 1e-3) const
    {
        return manipulability(jointPos) < threshold;
    }

    // --- 坐标系管理 ---

    /**
     * @brief 设置基坐标系偏移变换
     * @param tf 4x4 齐次变换矩阵，定义机器人基座相对于世界坐标系的位姿
     */
    void setBaseFrame(const Eigen::Matrix4d& tf) { baseTf_ = tf; }

    /**
     * @brief 设置工具坐标系(TCP)偏移变换
     * @param tf 4x4 齐次变换矩阵，定义末端执行器相对于法兰的位姿
     */
    void setToolFrame(const Eigen::Matrix4d& tf) { toolTf_ = tf; }

    /** @return 基坐标系偏移变换矩阵的常量引用 */
    const Eigen::Matrix4d& getBaseFrame() const { return baseTf_; }

    /** @return 工具坐标系(TCP)偏移变换矩阵的常量引用 */
    const Eigen::Matrix4d& getToolFrame() const { return toolTf_; }

    // --- 负载管理 ---

    /**
     * @brief 设置末端负载质量
     * @param mass 负载质量，单位 kg
     */
    void setPayload(double mass) { payloadMass_ = mass; }

    /** @return 末端负载质量 (kg) */
    double getPayload() const { return payloadMass_; }

    // --- 属性访问 ---

    /**
     * @brief 获取所有关节对应的物理轴ID列表
     * @return 轴ID向量的常量引用（首次调用时从 joints_ 缓存计算，后续零开销）
     */
    const std::vector<int>& getAxisIds() const;

    /** @return 自由度数量 */
    int getDof() const { return dof_; }

    /** @return 模型名称的常量引用 */
    const std::string& getName() const { return name_; }

    /** @return 模型类型字符串 ("serial" / "delta" / "cartesian") */
    const std::string& getType() const { return type_; }

    /** @return 模型关节列表的常量引用 */
    const std::vector<ModelJoint>& getJoints() const { return joints_; }

    /**
     * @brief 从平移+RPY构建4x4齐次变换矩阵
     */
    static Eigen::Matrix4d poseFromXYZRPY(double x, double y, double z,
                                           double rx, double ry, double rz);
};


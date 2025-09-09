#ifndef KINEMATICS_MODEL_H_
#define KINEMATICS_MODEL_H_

#include <vector>
#include <string>

/**
 * @brief 运动学模型接口基类
 * 
 * 定义了正向和逆向运动学计算的通用接口，
 * 以适配不同类型的机器人（如3轴、5轴、UR等）。
 */
class KinematicsModel 
{
public:
    virtual ~KinematicsModel() = default;

    /**
     * @brief 正向运动学计算
     * 
     * @param joint_positions 输入的关节角度
     * @param pose 输出的末端执行器位姿（例如，x, y, z, rx, ry, rz）
     * @return 计算是否成功
     */
    virtual bool forward(const std::vector<double>& joint_positions, std::vector<double>& pose) = 0;

    /**
     * @brief 逆向运动学计算
     * 
     * @param current_joints 当前的关节角度，用于选择最佳解
     * @param pose 输入的目标末端执行器位姿
     * @param target_joints 输出的计算得到的关节角度
     * @return 计算是否成功
     */
    virtual bool inverse(const std::vector<double>& current_joints, const std::vector<double>& pose, std::vector<double>& target_joints) = 0;

    /**
     * @brief 获取模型的自由度
     */
    virtual int getDOF() const = 0;

    /**
     * @brief 获取模型的名称
     */
    virtual std::string getName() const = 0;
};

#endif // KINEMATICS_MODEL_H_
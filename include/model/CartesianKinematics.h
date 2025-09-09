#ifndef CARTESIANKINEMATICS_H_
#define CARTESIANKINEMATICS_H_

#include "model/KinematicsModel.h"

class CartesianKinematics : public KinematicsModel
{
public:
    CartesianKinematics() = default;
    ~CartesianKinematics() = default;

    // 正向运动学：关节空间 -> 笛卡尔空间
    bool forwardKinematics(const std::vector<double>& joint_positions, std::vector<double>& cartesian_pose) override
    {
        if (joint_positions.size() != 3 || cartesian_pose.size() != 3)
        {
            return false; // 只支持3轴
        }
        // 对于笛卡尔机器人，关节位置直接对应笛卡尔位置
        cartesian_pose = joint_positions;
        return true;
    }

    // 逆向运动学：笛卡尔空间 -> 关节空间
    bool inverseKinematics(const std::vector<double>& cartesian_pose, std::vector<double>& joint_positions) override
    {
        if (cartesian_pose.size() != 3 || joint_positions.size() != 3)
        {
            return false; // 只支持3轴
        }
        // 对于笛卡尔机器人，笛卡尔位置直接对应关节位置
        joint_positions = cartesian_pose;
        return true;
    }

    int getDegreesOfFreedom() const override
    {
        return 3;
    }

    std::string getModelName() const override
    {
        return "CartesianKinematics";
    }
};

#endif // CARTESIANKINEMATICS_H_
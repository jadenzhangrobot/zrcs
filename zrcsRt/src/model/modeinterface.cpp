/**
 * @file modeinterface.cpp
 * @brief 模式接口基类实现
 */
#include "model/modeinterface.h"

bool ModeInterface::forwardKinematics(const Eigen::VectorXd& jointPos,
                                       Eigen::Matrix4d& toolPose) const
{
    if (!model_) return false;
    return model_->forwardKinematics(jointPos, toolPose);
}

bool ModeInterface::inverseKinematics(const Eigen::Matrix4d& toolPose,
                                       const Eigen::VectorXd& currentJointPos,
                                       Eigen::VectorXd& targetJointPos) const
{
    if (!model_) return false;
    return model_->inverseKinematics(toolPose, currentJointPos, targetJointPos);
}

bool ModeInterface::jacobian(const Eigen::VectorXd& jointPos,
                              Eigen::MatrixXd& J) const
{
    if (!model_) return false;
    return model_->jacobian(jointPos, J);
}

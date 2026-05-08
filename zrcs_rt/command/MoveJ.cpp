/*
 * @Description: 笛卡尔目�?关节路径PTP运动（ABB MoveJ）�?经IK解算
 */
#include "command/MoveJ.h"

MoveJ::MoveJ() : dof_(0) {
    std::strcpy(nodeName_, "MoveJ");
    axisIds_.reserve(zrcs::kAxisMax);
    // 设置 Ruckig 轨迹规划
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

}


bool MoveJ::initTrajectory()
{
    auto* registry = modelRegistry_;
    if (!registry) { ERROR_PRINT("MoveJ: 模型注册表未初始化\n"); return false;}
    RobotModel* model = registry->getModel(0);
    if (!model) { ERROR_PRINT("MoveJ: 未找到模型(id=0)\n"); return false;}

    if (axisIds_.empty())
    {
        dof_ = model->getDof();
        axisIds_ = model->getAxisIds();     // const ref → copy into pre-reserved buffer, zero realloc
    }

    // 构建目标位姿
    Eigen::Matrix4d targetPose = RobotModel::poseFromXYZRPY(
        command_->args[static_cast<size_t>(MoveJArg::X)],
        command_->args[static_cast<size_t>(MoveJArg::Y)],
        command_->args[static_cast<size_t>(MoveJArg::Z)],
        command_->args[static_cast<size_t>(MoveJArg::RX)],
        command_->args[static_cast<size_t>(MoveJArg::RY)],
        command_->args[static_cast<size_t>(MoveJArg::RZ)]);

    // 当前关节位置
    Eigen::VectorXd currentJoint(dof_);
    for (int i = 0; i < dof_; i++)
    {
        currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
    }

    // 逆运动学求解
    Eigen::VectorXd targetJoint(dof_);
    if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
    {
        ERROR_PRINT("MoveJ: IK 求解失败\n");
        return false;
    }

    

    double velScale = command_->args[static_cast<size_t>(MoveJArg::Vel)];
    if (velScale <= 0) velScale = 1.0;

    for (int i = 0; i < dof_; i++)
    {
        int axisId = axisIds_[i];
        input_->current_position[i] = currentJoint(i);
        input_->current_velocity[i] = 0;
        input_->current_acceleration[i] = 0;
        input_->target_position[i] = targetJoint(i);
        input_->target_velocity[i] = 0;
        input_->target_acceleration[i] = 0;
        input_->max_velocity[i] = controller_->axiss[axisId]->getMaxVelocity() * velScale;
        input_->max_acceleration[i] = controller_->axiss[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axiss[axisId]->getMaxJerk();
    }
    return true;
}

void MoveJ::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
}

CMD_REGISTER(MoveJ);

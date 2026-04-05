/*
 * @Description: 关节运动+位置触发IO（ABB TriggJ）
 */
#include "command/TriggJ.h"

bool TriggJ::initTrajectory()
{
    trigDist_ = command_->args[TriggJTrigDist];
    ioModule_ = static_cast<int>(command_->args[TriggJIOModule]);
    ioBit_ = static_cast<int>(command_->args[TriggJIOBit]);
    ioVal_ = static_cast<bool>(command_->args[TriggJIOVal]);
    triggered_ = false;

    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        ERROR_PRINT("TriggJ: 模型注册表未初始化\n");
        return false;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("TriggJ: 未找到模型(id=0)\n");
        return false;
    }

    dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    Eigen::Matrix4d targetPose = RobotModel::poseFromXYZRPY(
        command_->args[TriggJX], command_->args[TriggJY], command_->args[TriggJZ],
        command_->args[TriggJRX], command_->args[TriggJRY], command_->args[TriggJRZ]);

    Eigen::VectorXd currentJoint(dof_);
    for (int i = 0; i < dof_; i++)
    {
        currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
    }

    targetJoint_.resize(dof_);
    if (!model->inverseKinematics(targetPose, currentJoint, targetJoint_))
    {
        ERROR_PRINT("TriggJ: IK 求解失败\n");
        return false;
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double velScale = command_->args[TriggJVel];
    if (velScale <= 0) velScale = 1.0;

    for (int i = 0; i < dof_; i++)
    {
        int axisId = axisIds_[i];
        input_->current_position[i] = currentJoint(i);
        input_->current_velocity[i] = 0;
        input_->current_acceleration[i] = 0;
        input_->target_position[i] = targetJoint_(i);
        input_->target_velocity[i] = 0;
        input_->target_acceleration[i] = 0;
        input_->max_velocity[i] = controller_->axiss[axisId]->getMaxVelocity() * velScale;
        input_->max_acceleration[i] = controller_->axiss[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axiss[axisId]->getMaxJerk();
    }
    return true;
}

void TriggJ::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
}

void TriggJ::run(void)
{
    // 检查是否到达触发距离
    if (!triggered_)
    {
        double distSq = 0;
        for (int i = 0; i < dof_; i++)
        {
            double diff = targetJoint_(i) - controller_->axiss[axisIds_[i]]->actualPos();
            distSq += diff * diff;
        }
        if (std::sqrt(distSq) <= trigDist_)
        {
            if (ioModule_ >= 0 &&
                ioModule_ < static_cast<int>(controller_->ios_.size()))
            {
                controller_->ios_[ioModule_]->ioWrite32(ioModule_, ioBit_, ioVal_);
            }
            triggered_ = true;
        }
    }

    updateOverride();

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working)
    {
        applyOutput();
        output_->pass_to_input(*input_);
    }
    else if (result == Result::Finished)
    {
        applyOutput();
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("TriggJ: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

REGISTERCMD(TriggJ);

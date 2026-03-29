/*
 * @Description: 关节运动+位置触发IO（ABB TriggJ）
 */
#include "command/TriggJ.h"

void TriggJ::init()
{
    trigDist_ = command_->args[TriggJTrigDist];
    ioModule_ = static_cast<int>(command_->args[TriggJIOModule]);
    ioBit_ = static_cast<int>(command_->args[TriggJIOBit]);
    ioVal_ = static_cast<bool>(command_->args[TriggJIOVal]);
    triggered_ = false;

    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
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
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double override = shm().overrideRatio().load(std::memory_order_acquire);
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
        input_->max_velocity[i] = controller_->axiss[axisId]->getMaxVelocity() * override * velScale;
        input_->max_acceleration[i] = controller_->axiss[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axiss[axisId]->getMaxJerk();
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
            // 触发IO
            if (ioModule_ >= 0 &&
                ioModule_ < static_cast<int>(controller_->ios_.size()))
            {
                controller_->ios_[ioModule_]->ioWrite32(ioModule_, ioBit_, ioVal_);
            }
            triggered_ = true;
        }
    }

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
        }
        output_->pass_to_input(*input_);
    }
    else if (result == Result::Finished)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void TriggJ::exit(void) {}

REGISTERCMD(TriggJ);

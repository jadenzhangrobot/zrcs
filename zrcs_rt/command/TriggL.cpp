/*
 * @Description: 直线运动+位置触发IO（ABB TriggL）
 */
#include "command/TriggL.h"

TriggL::TriggL() : dof_(0), trigDist_(0), ioModule_(0), ioBit_(0), ioVal_(false), triggered_(false) { std::strcpy(nodeName_, "TriggL"); }
Result TriggL::updateTrajectory() { return otg_->update(*input_, *output_); }
void TriggL::passOutputToInput() { output_->pass_to_input(*input_); }
void TriggL::applyDeltaTime(double dt) { otg_->delta_time = dt; }

bool TriggL::initTrajectory()
{
    trigDist_ = command_->args[TriggLTrigDist];
    ioModule_ = static_cast<int>(command_->args[TriggLIOModule]);
    ioBit_ = static_cast<int>(command_->args[TriggLIOBit]);
    ioVal_ = static_cast<bool>(command_->args[TriggLIOVal]);
    triggered_ = false;

    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        ERROR_PRINT("TriggL: 模型注册表未初始化\n");
        return false;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("TriggL: 未找到模型(id=0)\n");
        return false;
    }

    dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    Eigen::Matrix4d targetPose = RobotModel::poseFromXYZRPY(
        command_->args[TriggLX], command_->args[TriggLY], command_->args[TriggLZ],
        command_->args[TriggLRX], command_->args[TriggLRY], command_->args[TriggLRZ]);

    Eigen::VectorXd currentJoint(dof_);
    for (int i = 0; i < dof_; i++)
    {
        currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
    }

    targetJoint_.resize(dof_);
    if (!model->inverseKinematics(targetPose, currentJoint, targetJoint_))
    {
        ERROR_PRINT("TriggL: IK 求解失败\n");
        return false;
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double velScale = command_->args[TriggLVel];
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

void TriggL::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
}

void TriggL::run(void)
{
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
        ERROR_PRINT("TriggL: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

REGISTERCMD(TriggL);

/*
 * @Description: 笛卡尔直线运动（ABB MoveL）— 经IK解算
 */
#include "command/MoveL.h"

void MoveL::init()
{
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
        command_->args[MoveLX], command_->args[MoveLY], command_->args[MoveLZ],
        command_->args[MoveLRX], command_->args[MoveLRY], command_->args[MoveLRZ]);

    Eigen::VectorXd currentJoint(dof_);
    for (int i = 0; i < dof_; i++)
    {
        currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
    }

    Eigen::VectorXd targetJoint(dof_);
    if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
    {
        ERROR_PRINT("MoveL: IK 求解失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double override = shm().overrideRatio().load(std::memory_order_acquire);
    double velScale = command_->args[MoveLVel];
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
        input_->max_velocity[i] = controller_->axiss[axisId]->getMaxVelocity() * override * velScale;
        input_->max_acceleration[i] = controller_->axiss[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axiss[axisId]->getMaxJerk();
    }
}

void MoveL::run(void)
{
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

void MoveL::exit(void) {}

REGISTERCMD(MoveL);

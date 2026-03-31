/*
 * @Description: 笛卡尔目标+关节路径PTP运动（ABB MoveJ）— 经IK解算
 */
#include "command/MoveJ.h"

void MoveJ::init()
{
    // 获取运动学模型
    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        ERROR_PRINT("MoveJ: 模型注册表未初始化\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("MoveJ: 未找到模型(id=0)\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    dof_ = model->getDof();
    auto axisIds = model->getAxisIds();

    // 构建目标位姿
    Eigen::Matrix4d targetPose = RobotModel::poseFromXYZRPY(
        command_->args[MoveJX], command_->args[MoveJY], command_->args[MoveJZ],
        command_->args[MoveJRX], command_->args[MoveJRY], command_->args[MoveJRZ]);

    // 当前关节位置
    Eigen::VectorXd currentJoint(dof_);
    for (int i = 0; i < dof_; i++)
    {
        currentJoint(i) = controller_->axiss[axisIds[i]]->actualPos();
    }

    // 逆运动学求解
    Eigen::VectorXd targetJoint(dof_);
    if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
    {
        ERROR_PRINT("MoveJ: IK 求解失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    // 设置 Ruckig 轨迹规划
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double override = shm().overrideRatio().load(std::memory_order_acquire);
    double velScale = command_->args[MoveJVel];
    if (velScale <= 0) velScale = 1.0;

    for (int i = 0; i < dof_; i++)
    {
        int axisId = axisIds[i];
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

void MoveJ::run(void)
{
    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    RobotModel* model = registry->getModel(0);
    auto axisIds = model->getAxisIds();

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[axisIds[i]]->setAxisPositionCmd(output_->new_position[i]);
        }
        output_->pass_to_input(*input_);
    }
    else if (result == Result::Finished)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[axisIds[i]]->setAxisPositionCmd(output_->new_position[i]);
        }
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("MoveJ: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void MoveJ::exit(void) {}

REGISTERCMD(MoveJ);

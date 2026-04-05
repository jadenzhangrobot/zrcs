/*
 * @Description: 直线搜索运动（ABB SearchL）— 运动中检测IO后停止记录位置
 */
#include "command/SearchL.h"

SearchL::SearchL() : dof_(0), ioModule_(0), ioBit_(0) { std::strcpy(nodeName_, "SearchL"); }
Result SearchL::updateTrajectory() { return otg_->update(*input_, *output_); }
void SearchL::passOutputToInput() { output_->pass_to_input(*input_); }
void SearchL::applyDeltaTime(double dt) { otg_->delta_time = dt; }

bool SearchL::initTrajectory()
{
    shm().probeTriggered().store(false, std::memory_order_release);

    ioModule_ = static_cast<int>(command_->args[SearchLIOModule]);
    ioBit_ = static_cast<int>(command_->args[SearchLIOBit]);

    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        ERROR_PRINT("SearchL: 模型注册表未初始化\n");
        return false;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("SearchL: 未找到模型(id=0)\n");
        return false;
    }

    dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    Eigen::Matrix4d targetPose = RobotModel::poseFromXYZRPY(
        command_->args[SearchLX], command_->args[SearchLY], command_->args[SearchLZ],
        command_->args[SearchLRX], command_->args[SearchLRY], command_->args[SearchLRZ]);

    Eigen::VectorXd currentJoint(dof_);
    for (int i = 0; i < dof_; i++)
    {
        currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
    }

    Eigen::VectorXd targetJoint(dof_);
    if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
    {
        ERROR_PRINT("SearchL: IK 求解失败\n");
        return false;
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double velScale = command_->args[SearchLVel];
    if (velScale <= 0) velScale = 0.1;  // 搜索运动默认低速

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

void SearchL::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
}

void SearchL::run(void)
{
    // 每周期检查IO信号
    if (ioModule_ >= 0 &&
        ioModule_ < static_cast<int>(controller_->ios_.size()))
    {
        if (controller_->ios_[ioModule_]->ioRead32(ioModule_, ioBit_))
        {
            // IO触发：记录当前位置到共享内存
            auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
            RobotModel* model = registry->getModel(0);

            Eigen::VectorXd jointPos(dof_);
            for (int i = 0; i < dof_; i++)
            {
                jointPos(i) = controller_->axiss[axisIds_[i]]->actualPos();
            }

            Eigen::Matrix4d toolPose;
            if (model->forwardKinematics(jointPos, toolPose))
            {
                double* result = shm().probeResult();
                result[0] = toolPose(0, 3);  // X
                result[1] = toolPose(1, 3);  // Y
                result[2] = toolPose(2, 3);  // Z
            }
            shm().probeTriggered().store(true, std::memory_order_release);

            // 停在当前位置
            for (int i = 0; i < dof_; i++)
            {
                controller_->axiss[axisIds_[i]]->setAxisPositionCmd(
                    controller_->axiss[axisIds_[i]]->actualPos());
            }
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
            return;
        }
    }

    updateOverride();

    // 继续运动
    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working)
    {
        applyOutput();
        output_->pass_to_input(*input_);
    }
    else if (result == Result::Finished)
    {
        // 到达终点但未触发IO
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("SearchL: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

REGISTERCMD(SearchL);

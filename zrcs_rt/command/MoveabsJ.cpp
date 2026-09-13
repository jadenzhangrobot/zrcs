/*
 * @Description: 绝对关节位置运动（ABB MoveAbsJ）�?多轴同步，不经IK
 */
#include "command/MoveabsJ.h"

#include <cmath>

MoveAbsJ::MoveAbsJ() : dof_(0) { std::strcpy(nodeName_, "MoveAbsJ"); }

bool MoveAbsJ::prepare()
{
    if (prepared_)
    {
        return true;
    }
    if (!controller_)
    {
        ERROR_PRINT("MoveAbsJ: controller is unavailable during prepare\n");
        return false;
    }
    auto* registry = modelRegistry_;
    if (!registry)
    {
        ERROR_PRINT("MoveAbsJ: 模型注册表未初始化\n");
        return false;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("MoveAbsJ: 未找到模�?id=0)\n");
        return false;
    }

    preparedDof_ = model->getDof();
    axisIds_ = model->getAxisIds();
    if (preparedDof_ <= 0 || axisIds_.size() != static_cast<size_t>(preparedDof_))
    {
        ERROR_PRINT("MoveAbsJ: invalid model DOF or axis mapping\n");
        return false;
    }
    for (int i = 0; i < preparedDof_; ++i)
    {
        const int axisId = axisIds_[static_cast<size_t>(i)];
        if (axisId < 0 || axisId >= static_cast<int>(controller_->axes_.size()) ||
            !controller_->axes_[axisId])
        {
            ERROR_PRINT("MoveAbsJ: axis id=%d is out of range during prepare\n", axisId);
            return false;
        }
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(preparedDof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(preparedDof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(preparedDof_);
    prepared_ = true;
    return true;
}

bool MoveAbsJ::initTrajectory()
{
    if (!prepared_ || !command_ || !otg_ || !input_ || !output_)
    {
        ERROR_PRINT("MoveAbsJ: prepared resources are unavailable\n");
        return false;
    }

    const double countValue = command_->args[static_cast<size_t>(MoveAbsJArg::Count)];
    if (!std::isfinite(countValue) || countValue < 0.0 ||
        std::floor(countValue) != countValue || countValue > preparedDof_)
    {
        ERROR_PRINT("MoveAbsJ: invalid Count %.6f for model DOF=%d\n",
                    countValue, preparedDof_);
        return false;
    }
    dof_ = countValue == 0.0 ? preparedDof_ : static_cast<int>(countValue);
    otg_->reset();

    for (int i = 0; i < preparedDof_; i++)
    {
        int axisId = axisIds_[i];
        input_->enabled[static_cast<size_t>(i)] = i < dof_;
        input_->current_position[i] = controller_->axes_[axisId]->actualPos();
        input_->current_velocity[i] = 0;
        input_->current_acceleration[i] = 0;
        input_->target_position[i] = i < dof_
            ? command_->args[static_cast<size_t>(MoveAbsJArg::J1) + i]
            : input_->current_position[i];
        input_->target_velocity[i] = 0;
        input_->target_acceleration[i] = 0;
        input_->max_velocity[i] = controller_->axes_[axisId]->getMaxVelocity();
        input_->max_acceleration[i] = controller_->axes_[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axes_[axisId]->getMaxJerk();
    }
    return true;
}

bool MoveAbsJ::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axes_[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
    return true;
}

REGISTERCMD(MoveAbsJ);

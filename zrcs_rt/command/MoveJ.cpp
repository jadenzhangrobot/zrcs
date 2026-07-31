/*
 * @Description: 笛卡尔目�?关节路径PTP运动（ABB MoveJ）�?经IK解算
 */
#include "command/MoveJ.h"

#include <cmath>

MoveJ::MoveJ() : dof_(0) {
    std::strcpy(nodeName_, "MoveJ");
    axisIds_.reserve(zrcs::kAxisMax);
    // Ruckig 在 initTrajectory 中按实际 DOF 延迟创建（构造函数中 dof_ 未知）
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
        otg_   = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
        input_  = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
        output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);
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
        currentJoint(i) = controller_->axes_[axisIds_[i]]->actualPos();
    }

    // 逆运动学求解
    Eigen::VectorXd targetJoint(dof_);
    if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
    {
        ERROR_PRINT("MoveJ: IK 求解失败 target=(%.4f,%.4f,%.4f; %.4f,%.4f,%.4f)\n",
                    targetPose(0,3), targetPose(1,3), targetPose(2,3),
                    command_->args[static_cast<size_t>(MoveJArg::RX)],
                    command_->args[static_cast<size_t>(MoveJArg::RY)],
                    command_->args[static_cast<size_t>(MoveJArg::RZ)]);
        return false;
    }

    // IK 模型有自己的几何限位，控制器仍必须按运行时 axis.xml 做最后一道
    // 防线。这样即使模型参数或行为树端口配置错误，也不会把越界目标交给
    // 软限位钳制后继续执行下一条 MoveL。
    for (int i = 0; i < dof_; ++i)
    {
        const int axisId = axisIds_[i];
        const double lower = controller_->axes_[axisId]->getNegativeLimit();
        const double upper = controller_->axes_[axisId]->getPositiveLimit();
        if (!std::isfinite(targetJoint(i)) || targetJoint(i) < lower - 1e-8 ||
            targetJoint(i) > upper + 1e-8)
        {
            ERROR_PRINT("MoveJ: axis%d IK 目标 %.6f 超出限位 [%.6f, %.6f]\n",
                        axisId, targetJoint(i), lower, upper);
            return false;
        }
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
        input_->max_velocity[i] = controller_->axes_[axisId]->getMaxVelocity() * velScale;
        input_->max_acceleration[i] = controller_->axes_[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axes_[axisId]->getMaxJerk();
    }
    return true;
}

bool MoveJ::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axes_[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
    return true;
}

CMD_REGISTER(MoveJ);

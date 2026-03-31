/*
 * @Description: 螺旋线插补（固高 GT_HelixXYZ）
 *               XY平面圆弧 + Z轴直线联动
 */
#include "command/HelixMove.h"

void HelixMove::init()
{
    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        ERROR_PRINT("HelixMove: 模型注册表未初始化\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("HelixMove: 未找到模型(id=0)\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    // get current position via FK
    Eigen::VectorXd currentJoint(dof_);
    for (int i = 0; i < dof_; i++)
    {
        currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
    }

    Eigen::Matrix4d curPose;
    if (!model->forwardKinematics(currentJoint, curPose))
    {
        ERROR_PRINT("HelixMove: FK 求解失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    double startX = curPose(0, 3);
    double startY = curPose(1, 3);
    zStart_ = command_->args[HelixMoveZStart];
    zEnd_ = command_->args[HelixMoveZEnd];

    double cx = command_->args[HelixMoveCenterX];
    double cy = command_->args[HelixMoveCenterY];
    center_ = Eigen::Vector2d(cx, cy);

    double endX = command_->args[HelixMoveEndX];
    double endY = command_->args[HelixMoveEndY];

    radius_ = std::sqrt((startX - cx) * (startX - cx) + (startY - cy) * (startY - cy));
    startAngle_ = std::atan2(startY - cy, startX - cx);
    double endAngle = std::atan2(endY - cy, endX - cx);

    totalAngle_ = endAngle - startAngle_;
    if (totalAngle_ <= 0) totalAngle_ += 2.0 * M_PI;

    // Ruckig for angle parameter [0, totalAngle_]
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);

    double override = shm().overrideRatio().load(std::memory_order_acquire);
    double velScale = command_->args[HelixMoveVel];
    if (velScale <= 0) velScale = 1.0;

    input_->current_position[0] = 0;
    input_->current_velocity[0] = 0;
    input_->current_acceleration[0] = 0;
    input_->target_position[0] = totalAngle_;
    input_->target_velocity[0] = 0;
    input_->target_acceleration[0] = 0;
    input_->max_velocity[0] = 2.0 * override * velScale;
    input_->max_acceleration[0] = 4.0;
    input_->max_jerk[0] = 20.0;
}

void HelixMove::run(void)
{
    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    RobotModel* model = registry->getModel(0);

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working || result == Result::Finished)
    {
        double theta = output_->new_position[0];
        double t = theta / totalAngle_;

        double x = center_.x() + radius_ * std::cos(startAngle_ + theta);
        double y = center_.y() + radius_ * std::sin(startAngle_ + theta);
        double z = zStart_ + t * (zEnd_ - zStart_);

        // IK
        Eigen::VectorXd currentJoint(dof_);
        for (int i = 0; i < dof_; i++)
        {
            currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
        }

        Eigen::Matrix4d curPose;
        model->forwardKinematics(currentJoint, curPose);
        Eigen::Matrix4d targetPose = curPose;
        targetPose(0, 3) = x;
        targetPose(1, 3) = y;
        targetPose(2, 3) = z;

        Eigen::VectorXd targetJoint(dof_);
        if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
        {
            ERROR_PRINT("HelixMove: IK 求解失败\n");
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            return;
        }

        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[axisIds_[i]]->setAxisPositionCmd(targetJoint(i));
        }

        if (result == Result::Finished)
        {
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        }
        else
        {
            output_->pass_to_input(*input_);
        }
    }
    else
    {
        ERROR_PRINT("HelixMove: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void HelixMove::exit(void) {}

REGISTERCMD(HelixMove);

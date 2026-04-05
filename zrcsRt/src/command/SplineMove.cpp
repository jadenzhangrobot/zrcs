/*
 * @Description: 样条曲线插补（正运动 MSPLINE）
 *               经过指定路径点的平滑样条曲线
 */
#include "command/SplineMove.h"

bool SplineMove::initTrajectory()
{
    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        ERROR_PRINT("SplineMove: 模型注册表未初始化\n");
        return false;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("SplineMove: 未找到模型(id=0)\n");
        return false;
    }

    dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    Eigen::Vector3d targetXYZ(
        command_->args[SplineMoveX],
        command_->args[SplineMoveY],
        command_->args[SplineMoveZ]);

    Eigen::VectorXd currentJoint(dof_);
    for (int i = 0; i < dof_; i++)
    {
        currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
    }

    Eigen::Matrix4d curPose;
    if (!model->forwardKinematics(currentJoint, curPose))
    {
        ERROR_PRINT("SplineMove: FK 求解失败\n");
        return false;
    }

    Eigen::Matrix4d targetPose = curPose;
    targetPose(0, 3) = targetXYZ.x();
    targetPose(1, 3) = targetXYZ.y();
    targetPose(2, 3) = targetXYZ.z();

    Eigen::VectorXd targetJoint(dof_);
    if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
    {
        ERROR_PRINT("SplineMove: IK 求解失败\n");
        return false;
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double velScale = command_->args[SplineMoveVel];
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

void SplineMove::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
}

REGISTERCMD(SplineMove);

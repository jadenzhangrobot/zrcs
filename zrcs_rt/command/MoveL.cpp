/*
 * @Description: 笛卡尔直线运动（ABB MoveL）— 经IK解算
 */
#include "command/MoveL.h"

MoveL::MoveL() : dof_(0) { std::strcpy(nodeName_, "MoveL"); }
Result MoveL::updateTrajectory() { return otg_->update(*input_, *output_); }
void MoveL::passOutputToInput() { output_->pass_to_input(*input_); }
void MoveL::applyDeltaTime(double dt) { otg_->delta_time = dt; }

bool MoveL::initTrajectory()
{
    auto* registry = modelRegistry_;
    if (!registry)
    {
        ERROR_PRINT("MoveL: 模型注册表未初始化\n");
        return false;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("MoveL: 未找到模型(id=0)\n");
        return false;
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
        return false;
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double vel = command_->args[MoveLVel];  // 笛卡尔速度 (mm/s)，由速度前瞻输出

    // 计算笛卡尔距离
    Eigen::Vector3d currentPos;
    {
        Eigen::Matrix4d curPose;
        model->forwardKinematics(currentJoint, curPose);
        currentPos = curPose.block<3,1>(0,3);
    }
    Eigen::Vector3d targetPos(command_->args[MoveLX], command_->args[MoveLY], command_->args[MoveLZ]);
    double cartDist = (targetPos - currentPos).norm();

    // 计算关节空间目标速度
    // vel > 0 表示连续路径模式：到达目标点时保持速度（不减速到零）
    // vel <= 0 表示点到点模式：到达目标点时减速到零
    Eigen::VectorXd jointDelta = targetJoint - currentJoint;
    double jointDist = jointDelta.norm();

    for (int i = 0; i < dof_; i++)
    {
        int axisId = axisIds_[i];
        input_->current_position[i] = currentJoint(i);
        input_->current_velocity[i] = 0;
        input_->current_acceleration[i] = 0;
        input_->target_position[i] = targetJoint(i);
        input_->target_acceleration[i] = 0;

        if (vel > 0 && cartDist > 1e-6 && jointDist > 1e-9)
        {
            // 连续路径：将笛卡尔速度映射到关节速度
            // v_joint[i] = vel * (delta_q[i] / |delta_cart|)
            double targetVel = vel * jointDelta(i) / cartDist;
            input_->target_velocity[i] = targetVel;
            input_->max_velocity[i] = controller_->axiss[axisId]->getMaxVelocity();
        }
        else
        {
            // 点到点：减速到零
            input_->target_velocity[i] = 0;
            double velScale = (vel > 0) ? vel : 1.0;
            input_->max_velocity[i] = controller_->axiss[axisId]->getMaxVelocity() * velScale;
        }

        input_->max_acceleration[i] = controller_->axiss[axisId]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axiss[axisId]->getMaxJerk();
    }
    return true;
}

void MoveL::applyOutput()
{
    for (int i = 0; i < dof_; i++)
    {
        controller_->axiss[axisIds_[i]]->setAxisPositionCmd(output_->new_position[i]);
    }
}

REGISTERCMD(MoveL, 25);

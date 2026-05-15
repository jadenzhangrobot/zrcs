/*
 * @Description: 圆弧运动（ABB MoveC）�?三点定弧，经IK解算
 */
#include "command/MoveC.h"

MoveC::MoveC() : dof_(0), radius_(0), totalAngle_(0), zStart_(0), zEnd_(0) { std::strcpy(nodeName_, "MoveC"); }
void MoveC::applyOutput() {}


bool MoveC::initTrajectory()
{
    auto* registry = modelRegistry_;
    if (!registry)
    {
        ERROR_PRINT("MoveC: 模型注册表未初始化\n");
        return false;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("MoveC: 未找到模�?id=0)\n");
        return false;
    }

    dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    Eigen::VectorXd currentJoint(dof_);
    for (int i = 0; i < dof_; i++)
    {
        currentJoint(i) = controller_->axes_[axisIds_[i]]->actualPos();
    }

    // FK: current joint -> start cartesian position
    Eigen::Matrix4d startPose;
    if (!model->forwardKinematics(currentJoint, startPose))
    {
        ERROR_PRINT("MoveC: FK 求解失败\n");
        return false;
    }

    Eigen::Vector3d P0 = startPose.block<3,1>(0,3);
    Eigen::Vector3d P1(
        command_->args[static_cast<size_t>(MoveCArg::ViaX)],
        command_->args[static_cast<size_t>(MoveCArg::ViaY)],
        command_->args[static_cast<size_t>(MoveCArg::ViaZ)]);
    Eigen::Vector3d P2(
        command_->args[static_cast<size_t>(MoveCArg::EndX)],
        command_->args[static_cast<size_t>(MoveCArg::EndY)],
        command_->args[static_cast<size_t>(MoveCArg::EndZ)]);

    // three-point circle: compute center using circumcenter formula
    Eigen::Vector3d A = P1 - P0;
    Eigen::Vector3d B = P2 - P0;
    Eigen::Vector3d N = A.cross(B);
    double N2 = N.squaredNorm();
    if (N2 < 1e-12)
    {
        ERROR_PRINT("MoveC: 三点共线, 无法确定圆弧\n");
        return false;
    }

    Eigen::Vector3d d = P1 - P0;
    Eigen::Vector3d e = P2 - P0;
    double dd = d.dot(d);
    double ee = e.dot(e);
    double de = d.dot(e);
    double denom = 2.0 * (dd * ee - de * de);
    if (std::abs(denom) < 1e-12)
    {
        ERROR_PRINT("MoveC: 三点共线, 无法确定圆弧\n");
        return false;
    }
    double s = (dd * ee - ee * de) / denom;
    double t = (ee * dd - dd * de) / denom;
    center_ = P0 + s * d + t * e;

    radius_ = (P0 - center_).norm();
    axis_ = N.normalized();

    startRadial_ = (P0 - center_).normalized();
    biNormal_ = axis_.cross(startRadial_);

    // compute total angle from P0 to P2 via P1
    Eigen::Vector3d r1 = P1 - center_;
    Eigen::Vector3d r2 = P2 - center_;
    double angle1 = std::atan2(r1.dot(biNormal_), r1.dot(startRadial_));
    double angle2 = std::atan2(r2.dot(biNormal_), r2.dot(startRadial_));
    if (angle1 < 0) angle1 += 2.0 * M_PI;
    if (angle2 < 0) angle2 += 2.0 * M_PI;
    if (angle2 < angle1) angle2 += 2.0 * M_PI;
    totalAngle_ = angle2;

    startPos_ = P0;
    zStart_ = P0.z();
    zEnd_ = P2.z();

    // Use Ruckig for 1-DOF angle parameter [0, totalAngle_]
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);

    double velScale = command_->args[static_cast<size_t>(MoveCArg::Vel)];
    if (velScale <= 0) velScale = 1.0;

    input_->current_position[0] = 0;
    input_->current_velocity[0] = 0;
    input_->current_acceleration[0] = 0;
    input_->target_position[0] = totalAngle_;
    input_->target_velocity[0] = 0;
    input_->target_acceleration[0] = 0;
    input_->max_velocity[0] = 2.0 * velScale;
    input_->max_acceleration[0] = 4.0;
    input_->max_jerk[0] = 20.0;
    return true;
}

void MoveC::run()
{
    updateOverride();

    auto* registry = modelRegistry_;
    RobotModel* model = registry->getModel(0);

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working || result == Result::Finished)
    {
        double theta = output_->new_position[0];

        // cartesian position on arc
        Eigen::Vector3d pos = center_ + radius_ * (std::cos(theta) * startRadial_ + std::sin(theta) * biNormal_);

        // IK
        Eigen::VectorXd currentJoint(dof_);
        for (int i = 0; i < dof_; i++)
        {
            currentJoint(i) = controller_->axes_[axisIds_[i]]->actualPos();
        }

        // build target pose (keep current orientation for simplicity)
        Eigen::Matrix4d curPose;
        model->forwardKinematics(currentJoint, curPose);
        Eigen::Matrix4d targetPose = curPose;
        targetPose(0,3) = pos.x();
        targetPose(1,3) = pos.y();
        targetPose(2,3) = pos.z();

        Eigen::VectorXd targetJoint(dof_);
        if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
        {
            ERROR_PRINT("MoveC: IK 求解失败\n");
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            return;
        }

        for (int i = 0; i < dof_; i++)
        {
            controller_->axes_[axisIds_[i]]->setAxisPositionCmd(targetJoint(i));
        }

        if (result == Result::Finished)
        {
            setCmdStatus(zrcsSystem::CmdStatus::COMPLETED);
        }
        else
        {
            output_->pass_to_input(*input_);
        }
    }
    else
    {
        ERROR_PRINT("MoveC: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

CMD_REGISTER(MoveC);

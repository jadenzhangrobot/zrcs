#include "command/MovePath.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include "shared_memory/ShmLayout.h"
#include "system/node/NodeFactory.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

MovePath::MovePath()
{
    std::strcpy(nodeName_, "MovePath");
}

Eigen::Vector3d MovePath::evaluateArc(double u) const
{
    const double theta = arcSweep_ * u;
    return arcCenter_ + arcRadius_ * (std::cos(theta) * arcU_ + std::sin(theta) * arcV_);
}

namespace {

double unwrapNear(double angle, double reference)
{
    constexpr double kTwoPi = 2.0 * M_PI;
    return angle + kTwoPi * std::round((reference - angle) / kTwoPi);
}

bool isXyzacModel(const RobotModel& model)
{
    const auto& joints = model.getJoints();
    if (model.getType() != "xyzac_table" || joints.size() != 5)
    {
        return false;
    }

    const auto axisIs = [](const ModelJoint& joint, char axis, JointType type)
    {
        return joint.type == type &&
               std::toupper(static_cast<unsigned char>(joint.axis)) == axis;
    };

    return axisIs(joints[0], 'X', JointType::PRISMATIC) &&
           axisIs(joints[1], 'Y', JointType::PRISMATIC) &&
           axisIs(joints[2], 'Z', JointType::PRISMATIC) &&
           axisIs(joints[3], 'X', JointType::REVOLUTE) &&
           axisIs(joints[4], 'Z', JointType::REVOLUTE);
}

} // namespace

bool MovePath::prepare()
{
    if (prepared_)
    {
        return true;
    }
    if (!controller_ || !modelRegistry_)
    {
        ERROR_PRINT("MovePath: controller/model registry is unavailable during prepare\n");
        return false;
    }

    model_ = modelRegistry_->getModel(0);
    if (!model_)
    {
        ERROR_PRINT("MovePath: model id=0 not found during prepare\n");
        return false;
    }

    axisIds_ = model_->getAxisIds();
    dof_ = model_->getDof();
    if (axisIds_.size() < 3 || dof_ != static_cast<int>(axisIds_.size()))
    {
        ERROR_PRINT("MovePath: invalid model DOF or axis mapping\n");
        return false;
    }
    for (int i = 0; i < dof_; ++i)
    {
        const int axisId = axisIds_[static_cast<size_t>(i)];
        if (axisId < 0 || axisId >= static_cast<int>(controller_->axes_.size()) ||
            !controller_->axes_[axisId])
        {
            ERROR_PRINT("MovePath: axis id=%d is out of range during prepare\n", axisId);
            return false;
        }
    }

    rtcp5Axis_ = isXyzacModel(*model_);
    ikSeed_.resize(dof_);
    targetJoint_.resize(dof_);
    lastJointTarget_.resize(dof_);
    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(1);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);
    input_->min_velocity = std::vector<double>{0.0};
    prepared_ = true;
    return true;
}

bool MovePath::initializeRtcp()
{
    startQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MovePathArg::QStartW)],
        command_->args[static_cast<size_t>(MovePathArg::QStartX)],
        command_->args[static_cast<size_t>(MovePathArg::QStartY)],
        command_->args[static_cast<size_t>(MovePathArg::QStartZ)]);
    endQuat_ = Eigen::Quaterniond(
        command_->args[static_cast<size_t>(MovePathArg::QEndW)],
        command_->args[static_cast<size_t>(MovePathArg::QEndX)],
        command_->args[static_cast<size_t>(MovePathArg::QEndY)],
        command_->args[static_cast<size_t>(MovePathArg::QEndZ)]);

    const double startNorm = startQuat_.norm();
    const double endNorm = endQuat_.norm();
    if (!std::isfinite(startNorm) || !std::isfinite(endNorm) ||
        startNorm < 1e-12 || endNorm < 1e-12)
    {
        ERROR_PRINT("MovePath: quaternion is invalid\n");
        return false;
    }
    startQuat_.normalize();
    endQuat_.normalize();

    if (!rtcp5Axis_)
    {
        return true;
    }

    const Eigen::Matrix3d startRotation = startQuat_.toRotationMatrix();
    const Eigen::Matrix3d endRotation = endQuat_.toRotationMatrix();
    const auto& joints = model_->getJoints();

    // NRT sends conventional RPY (Rz * Ry * Rx); XYZAC accepts only ry=0.
    startA_ = unwrapNear(std::atan2(startRotation(2, 1), startRotation(2, 2)),
                         ikSeed_(3) + joints[3].offset);
    startC_ = unwrapNear(std::atan2(startRotation(1, 0), startRotation(0, 0)),
                         ikSeed_(4) + joints[4].offset);
    endA_ = unwrapNear(std::atan2(endRotation(2, 1), endRotation(2, 2)), startA_);
    endC_ = unwrapNear(std::atan2(endRotation(1, 0), endRotation(0, 0)), startC_);

    const Eigen::Matrix3d reconstructedStart =
        (Eigen::AngleAxisd(startC_, Eigen::Vector3d::UnitZ()) *
         Eigen::AngleAxisd(startA_, Eigen::Vector3d::UnitX())).toRotationMatrix();
    const Eigen::Matrix3d reconstructedEnd =
        (Eigen::AngleAxisd(endC_, Eigen::Vector3d::UnitZ()) *
         Eigen::AngleAxisd(endA_, Eigen::Vector3d::UnitX())).toRotationMatrix();
    if ((reconstructedStart - startRotation).norm() > 1e-8 ||
        (reconstructedEnd - endRotation).norm() > 1e-8)
    {
        ERROR_PRINT("MovePath: XYZAC orientation must be representable by A/C axes\n");
        return false;
    }

    return true;
}

bool MovePath::solveRtcp(const Eigen::Vector3d& pos, double u)
{
    const double a = startA_ + u * (endA_ - startA_);
    const double c = startC_ + u * (endC_ - startC_);

    Eigen::Matrix4d targetPose = Eigen::Matrix4d::Identity();
    // The machine model consumes its physical rotary chain: RotX(A) * RotZ(C).
    targetPose.block<3, 3>(0, 0) =
        (Eigen::AngleAxisd(a, Eigen::Vector3d::UnitX()) *
         Eigen::AngleAxisd(c, Eigen::Vector3d::UnitZ())).toRotationMatrix();
    targetPose.block<3, 1>(0, 3) = pos;

    ikSeed_ = lastJointTarget_;
    if (!model_->inverseKinematics(targetPose, ikSeed_, targetJoint_) ||
        targetJoint_.size() != dof_ || !targetJoint_.allFinite())
    {
        ERROR_PRINT("MovePath: XYZAC RTCP inverse kinematics failed\n");
        return false;
    }

    for (int i = 0; i < dof_; ++i)
    {
        controller_->axes_[axisIds_[i]]->setAxisPositionCmd(targetJoint_(i));
    }
    lastJointTarget_ = targetJoint_;
    jointTargetValid_ = true;
    return true;
}

bool MovePath::initTrajectory()
{
    if (!prepared_ || !command_ || !model_ || !otg_ || !input_ || !output_)
    {
        ERROR_PRINT("MovePath: command or prepared resources are unavailable\n");
        return false;
    }

    const bool sync = command_->args[static_cast<size_t>(MovePathArg::Sync)] == 1.0;
    if (sync)
    {
        arcOffset_ = 0.0;
        input_->current_position[0] = 0.0;
        input_->current_velocity[0] = 0.0;
        input_->current_acceleration[0] = 0.0;
    }

    if (sync || !jointTargetValid_)
    {
        for (int i = 0; i < dof_; ++i)
        {
            lastJointTarget_(i) = controller_->axes_[axisIds_[i]]->actualPos();
        }
        jointTargetValid_ = true;
    }
    ikSeed_ = lastJointTarget_;

    if (!initializeRtcp())
    {
        return false;
    }

    const int shape = static_cast<int>(command_->args[static_cast<size_t>(MovePathArg::Shape)]);
    isArc_ = shape == 1;

    if (isArc_)
    {
        arcCenter_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P0X)],
            command_->args[static_cast<size_t>(MovePathArg::P0Y)],
            command_->args[static_cast<size_t>(MovePathArg::P0Z)]);
        arcU_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P1X)],
            command_->args[static_cast<size_t>(MovePathArg::P1Y)],
            command_->args[static_cast<size_t>(MovePathArg::P1Z)]);
        arcV_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P2X)],
            command_->args[static_cast<size_t>(MovePathArg::P2Y)],
            command_->args[static_cast<size_t>(MovePathArg::P2Z)]);
        arcRadius_ = command_->args[static_cast<size_t>(MovePathArg::Radius)];
        arcSweep_ = command_->args[static_cast<size_t>(MovePathArg::Sweep)];

        const double uNorm = arcU_.norm();
        const double vNorm = arcV_.norm();
        if (!std::isfinite(arcRadius_) || !std::isfinite(arcSweep_) ||
            arcRadius_ <= 1e-9 || std::abs(arcSweep_) <= 1e-9 ||
            uNorm <= 1e-9 || vNorm <= 1e-9)
        {
            ERROR_PRINT("MovePath: circular arc args are invalid\n");
            return false;
        }
        arcU_ /= uNorm;
        arcV_ /= vNorm;

        startPos_ = evaluateArc(0.0);
        targetPos_ = evaluateArc(1.0);
        geometryLength_ = std::abs(arcRadius_ * arcSweep_);
    }
    else
    {
        // Shape=0：直线；其它 shape 当前协议不支持，按直线处理
        startPos_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P0X)],
            command_->args[static_cast<size_t>(MovePathArg::P0Y)],
            command_->args[static_cast<size_t>(MovePathArg::P0Z)]);
        targetPos_ = Eigen::Vector3d(
            command_->args[static_cast<size_t>(MovePathArg::P1X)],
            command_->args[static_cast<size_t>(MovePathArg::P1Y)],
            command_->args[static_cast<size_t>(MovePathArg::P1Z)]);
        geometryLength_ = (targetPos_ - startPos_).norm();
    }

    const double commandLength = command_->args[static_cast<size_t>(MovePathArg::Length)];
    if (!std::isfinite(commandLength) || commandLength < 1e-6)
    {
        ERROR_PRINT("MovePath: path length is zero or invalid\n");
        return false;
    }
    if (!std::isfinite(geometryLength_) || geometryLength_ < 1e-6)
    {
        ERROR_PRINT("MovePath: geometry length is zero or invalid\n");
        return false;
    }

    const double maxVel = command_->args[static_cast<size_t>(MovePathArg::Vel)];
    const double targetVel = command_->args[static_cast<size_t>(MovePathArg::TargetVel)];
    const double targetAcc = command_->args[static_cast<size_t>(MovePathArg::TargetAcc)];
    const double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    const double maxJerk = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);

    if (!std::isfinite(maxVel) || maxVel <= 0.0 ||
        !std::isfinite(targetVel) || targetVel < 0.0 ||
        !std::isfinite(maxAccel) || maxAccel <= 0.0 ||
        !std::isfinite(targetAcc) || std::abs(targetAcc) > maxAccel ||
        !std::isfinite(maxJerk) || maxJerk <= 0.0)
    {
        ERROR_PRINT("MovePath: velocity, acceleration or jerk limit is invalid\n");
        return false;
    }

    arcOffset_ = arcOffset_ + geometryLength_;
    input_->target_position[0] = arcOffset_;
    input_->target_velocity[0] = targetVel;
    input_->target_acceleration[0] = targetAcc;
    input_->max_velocity[0] = maxVel;
    input_->max_acceleration[0] = maxAccel;
    input_->max_jerk[0] = maxJerk;

    return true;
}

bool MovePath::applyOutput()
{
    // otg_->update() 已由 TrajectoryCmd::run() -> updateTrajectory() 完成
    const double s = output_->new_position[0];
    const double geometryS = s - (arcOffset_ - geometryLength_);
    const double u = geometryLength_ > 1e-13
                         ? geometryS / geometryLength_
                         : 0.0;

    Eigen::Vector3d pos;
    if (isArc_)
    {
        pos = evaluateArc(u);
    }
    else
    {
        pos = startPos_ + u * (targetPos_ - startPos_);
    }

    if (rtcp5Axis_)
    {
        return solveRtcp(pos, u);
    }

    controller_->axes_[axisIds_[0]]->setAxisPositionCmd(pos.x());
    controller_->axes_[axisIds_[1]]->setAxisPositionCmd(pos.y());
    controller_->axes_[axisIds_[2]]->setAxisPositionCmd(pos.z());
    return true;
}

REGISTERCMD(MovePath);

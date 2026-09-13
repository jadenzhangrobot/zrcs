#include "model/PandaRobot.h"

#include <Eigen/Geometry>
#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

Eigen::Matrix4d translate(double x, double y, double z)
{
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
    T.block<3, 1>(0, 3) << x, y, z;
    return T;
}

Eigen::Matrix4d rotateX(double angle)
{
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
    T.block<3, 3>(0, 0) = Eigen::AngleAxisd(angle, Eigen::Vector3d::UnitX()).toRotationMatrix();
    return T;
}

Eigen::Matrix4d rotateY(double angle)
{
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
    T.block<3, 3>(0, 0) = Eigen::AngleAxisd(angle, Eigen::Vector3d::UnitY()).toRotationMatrix();
    return T;
}

Eigen::Matrix4d rotateZ(double angle)
{
    Eigen::Matrix4d T = Eigen::Matrix4d::Identity();
    T.block<3, 3>(0, 0) = Eigen::AngleAxisd(angle, Eigen::Vector3d::UnitZ()).toRotationMatrix();
    return T;
}

} // namespace

PandaRobot::PandaRobot(const ModelParam& param)
    : RobotModel(param.name, "panda", 7),
      isRightArm_(param.name.find("right") != std::string::npos)
{
    joints_ = param.joints;
    baseTf_ = param.baseTf;
    toolTf_ = param.toolTf;
}

Eigen::Matrix4d PandaRobot::baseRotation() const
{
    // MuJoCo's default eulerseq is XYZ.  The GARMI MJCF uses
    // (1.57, 1.57, 3.14) on the left and (1.57, 1.57, 0) on the right.
    return rotateX(1.57) * rotateY(1.57) *
           rotateZ(isRightArm_ ? 0.0 : kPi);
}

bool PandaRobot::forwardKinematics(const Eigen::VectorXd& jointPos,
                                   Eigen::Matrix4d& toolPose) const
{
    if (jointPos.size() != 7 || joints_.size() != 7 || !jointPos.allFinite()) {
        return false;
    }

    Eigen::Matrix4d T = baseTf_ * baseRotation();
    T *= translate(0.0, 0.0, 0.333);
    T *= rotateZ(jointPos(0));
    T *= rotateX(-kPi / 2.0);
    T *= rotateZ(jointPos(1));
    T *= translate(0.0, -0.316, 0.0);
    T *= rotateX(kPi / 2.0);
    T *= rotateZ(jointPos(2));
    T *= translate(0.0825, 0.0, 0.0);
    T *= rotateX(kPi / 2.0);
    T *= rotateZ(jointPos(3));
    T *= translate(-0.0825, 0.384, 0.0);
    T *= rotateX(-kPi / 2.0);
    T *= rotateZ(jointPos(4));
    T *= rotateX(kPi / 2.0);
    T *= rotateZ(jointPos(5));
    T *= translate(0.088, 0.0, 0.0);
    T *= rotateX(kPi / 2.0);
    T *= rotateZ(jointPos(6));
    T *= translate(0.0, 0.0, 0.107);
    toolPose = T * toolTf_;
    return toolPose.allFinite();
}

bool PandaRobot::jacobian(const Eigen::VectorXd& jointPos,
                          Eigen::MatrixXd& J) const
{
    if (jointPos.size() != 7 || !jointPos.allFinite()) {
        return false;
    }

    constexpr double delta = 1e-7;
    Eigen::Matrix4d T0;
    if (!forwardKinematics(jointPos, T0)) {
        return false;
    }
    J.resize(6, 7);
    const Eigen::Vector3d p0 = T0.block<3, 1>(0, 3);
    const Eigen::Matrix3d R0 = T0.block<3, 3>(0, 0);
    for (int i = 0; i < 7; ++i) {
        Eigen::VectorXd q = jointPos;
        q(i) += delta;
        Eigen::Matrix4d Td;
        if (!forwardKinematics(q, Td)) {
            return false;
        }
        J.block<3, 1>(0, i) = (Td.block<3, 1>(0, 3) - p0) / delta;
        const Eigen::Matrix3d skew = ((Td.block<3, 3>(0, 0) - R0) / delta) * R0.transpose();
        J(3, i) = skew(2, 1);
        J(4, i) = skew(0, 2);
        J(5, i) = skew(1, 0);
    }
    return true;
}

bool PandaRobot::inverseKinematics(const Eigen::Matrix4d& toolPose,
                                   const Eigen::VectorXd& currentJointPos,
                                   Eigen::VectorXd& targetJointPos) const
{
    if (currentJointPos.size() != 7 || !toolPose.allFinite() ||
        !currentJointPos.allFinite()) {
        return false;
    }

    constexpr int maxIter = 100;
    constexpr double tolerance = 1e-5;
    constexpr double lambda = 0.02;
    Eigen::VectorXd q = currentJointPos;
    for (int iter = 0; iter < maxIter; ++iter) {
        Eigen::Matrix4d currentPose;
        if (!forwardKinematics(q, currentPose)) {
            return false;
        }
        Eigen::Matrix<double, 6, 1> error;
        error.head<3>() = toolPose.block<3, 1>(0, 3) - currentPose.block<3, 1>(0, 3);
        const Eigen::Matrix3d Rerror = toolPose.block<3, 3>(0, 0) *
                                       currentPose.block<3, 3>(0, 0).transpose();
        const Eigen::AngleAxisd aa(Rerror);
        error.tail<3>() = aa.angle() * aa.axis();
        if (!error.allFinite()) {
            return false;
        }
        if (error.norm() < tolerance) {
            targetJointPos = q;
            return true;
        }

        Eigen::MatrixXd J;
        if (!jacobian(q, J)) {
            return false;
        }
        const Eigen::MatrixXd JJt = J * J.transpose();
        const Eigen::MatrixXd damped = JJt +
            lambda * lambda * Eigen::MatrixXd::Identity(6, 6);
        q += J.transpose() * damped.ldlt().solve(error);
    }

    targetJointPos = q;
    return false;
}

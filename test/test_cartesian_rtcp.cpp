#include "model/XyzacTableRobot.h"
#include "model/ModelConfig.h"

#include <Eigen/Geometry>
#include <cassert>
#include <cmath>
#include <vector>

namespace {

ModelJoint joint(int axisId, JointType type, char axis)
{
    ModelJoint result;
    result.axisId = axisId;
    result.type = type;
    result.axis = axis;
    return result;
}

bool isNear(const Eigen::Matrix4d& lhs, const Eigen::Matrix4d& rhs,
            double tolerance = 1e-10)
{
    return (lhs - rhs).norm() <= tolerance;
}

} // namespace

int main()
{
    const ModelConfig config(std::string(ZRCS_SOURCE_DIR) + "/config/5axis/model.xml");
    assert(config.modelParams.size() == 1);
    assert(config.modelParams[0].type == "xyzac_table");
    assert(std::abs(config.modelParams[0].baseTf(2, 3) - 0.38) < 1e-12);
    assert(std::abs(config.modelParams[0].toolTf(2, 3) - 0.05) < 1e-12);
    assert(std::abs(config.modelParams[0].joints[2].offset - 0.6) < 1e-12);

    std::vector<ModelJoint> joints = {
        joint(0, JointType::PRISMATIC, 'X'),
        joint(1, JointType::PRISMATIC, 'Y'),
        joint(2, JointType::PRISMATIC, 'Z'),
        joint(3, JointType::REVOLUTE, 'X'),
        joint(4, JointType::REVOLUTE, 'Z'),
    };
    joints[2].offset = 0.6;

    Eigen::Matrix4d pivot = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d tableOffset = Eigen::Matrix4d::Identity();
    pivot(2, 3) = 0.38;
    tableOffset(2, 3) = 0.05;
    XyzacTableRobot model("xyzac", joints, pivot, tableOffset);

    constexpr double a = 1.8;
    constexpr double c = -0.7;
    Eigen::Matrix4d targetPose = Eigen::Matrix4d::Identity();
    targetPose.block<3, 1>(0, 3) = Eigen::Vector3d(0.03, -0.02, 0.17);
    targetPose.block<3, 3>(0, 0) =
        (Eigen::AngleAxisd(a, Eigen::Vector3d::UnitX()) *
         Eigen::AngleAxisd(c, Eigen::Vector3d::UnitZ())).toRotationMatrix();

    Eigen::VectorXd current = Eigen::VectorXd::Zero(5);
    Eigen::VectorXd command;
    assert(model.inverseKinematics(targetPose, current, command));
    assert(command.size() == 5);
    assert((command.head<3>() - targetPose.block<3, 1>(0, 3)).norm() > 1e-3);
    assert(std::abs(command(3) - a) < 1e-10);
    assert(std::abs(command(4) - c) < 1e-10);

    Eigen::Matrix4d actualPose;
    assert(model.forwardKinematics(command, actualPose));
    assert(isNear(actualPose, targetPose));

    current(4) = 3.2;
    targetPose.block<3, 3>(0, 0) =
        (Eigen::AngleAxisd(a, Eigen::Vector3d::UnitX()) *
         Eigen::AngleAxisd(-3.0, Eigen::Vector3d::UnitZ())).toRotationMatrix();
    assert(model.inverseKinematics(targetPose, current, command));
    assert(command(4) > M_PI);

    targetPose.block<3, 3>(0, 0) =
        Eigen::AngleAxisd(0.1, Eigen::Vector3d::UnitY()).toRotationMatrix();
    assert(!model.inverseKinematics(targetPose, current, command));
    return 0;
}

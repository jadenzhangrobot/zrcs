#pragma once

#include "model/RobotModel.h"
#include "model/ModelConfig.h"

/**
 * Kinematics for the Panda arm used by the GARMI MuJoCo model.
 *
 * The transforms are taken from the upstream MJCF body chain rather than an
 * approximate DH table, so Cartesian commands use the same frame convention
 * as the simulator.
 */
class PandaRobot final : public RobotModel
{
public:
    PandaRobot(const ModelParam& param);

    bool forwardKinematics(const Eigen::VectorXd& jointPos,
                           Eigen::Matrix4d& toolPose) const override;

    bool inverseKinematics(const Eigen::Matrix4d& toolPose,
                           const Eigen::VectorXd& currentJointPos,
                           Eigen::VectorXd& targetJointPos) const override;

    bool jacobian(const Eigen::VectorXd& jointPos,
                  Eigen::MatrixXd& J) const override;

private:
    Eigen::Matrix4d baseRotation() const;
    bool isRightArm_ = false;
};

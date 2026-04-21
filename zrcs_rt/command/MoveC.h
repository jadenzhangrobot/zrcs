/*
 * @Description: 圆弧运动（ABB MoveC）— 三点定弧，经IK解算
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <memory>
#include <cmath>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveC : public TrajectoryCmd
{
public:
    CMD_DEFINE(26,
        PARAM(ViaX)
        PARAM(ViaY)
        PARAM(ViaZ)
        PARAM(EndX)
        PARAM(EndY)
        PARAM(EndZ)
        PARAM(Vel)
    )

private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;

    Eigen::Vector3d center_;
    Eigen::Vector3d axis_;
    double radius_;
    double totalAngle_;
    Eigen::Vector3d startPos_;
    Eigen::Vector3d startRadial_;
    Eigen::Vector3d biNormal_;
    double zStart_;
    double zEnd_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override;
    void applyOutput() override;
    void passOutputToInput() override;
    void applyDeltaTime(double dt) override;

public:
    MoveC();
    void run() override;
};


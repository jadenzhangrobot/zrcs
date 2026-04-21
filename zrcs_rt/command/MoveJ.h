/*
 * @Description: 笛卡尔目标+关节路径PTP运动（ABB MoveJ）— 经IK解算
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <memory>
#include <Eigen/Dense>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveJ : public TrajectoryCmd
{
public:
    CMD_DEFINE(24,
        PARAM(X)
        PARAM(Y)
        PARAM(Z)
        PARAM(RX)
        PARAM(RY)
        PARAM(RZ)
        PARAM(Vel)
    )

private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;
    std::vector<int> axisIds_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override;
    void applyOutput() override;
    void passOutputToInput() override;
    void applyDeltaTime(double dt) override;

public:
    MoveJ();
};


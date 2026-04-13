/*
 * @Description: 绝对关节位置运动（ABB MoveAbsJ）— 多轴同步，不经IK
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include "model/ModelFactory.h"
#include "model/RobotModel.h"
#include <memory>
#include <ruckig/ruckig.hpp>

CMD_DEFINE(MoveAbsJ, 23,
    PARAM(Count)
    PARAM(J1)
    PARAM(J2)
    PARAM(J3)
    PARAM(J4)
    PARAM(J5)
    PARAM(J6)
)

using namespace ruckig;

class MoveAbsJ : public TrajectoryCmd
{
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
    MoveAbsJ();
};

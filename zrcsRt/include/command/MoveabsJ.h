/*
 * @Description: 绝对关节位置运动（ABB MoveAbsJ）— 多轴同步，不经IK
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/TrajectoryCmd.h"
#include "system/nodeFactory.h"
#include "model/modelFactory.h"
#include "model/robotModel.h"
#include <memory>
#include <ruckig/ruckig.hpp>

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
    Result updateTrajectory() override { return otg_->update(*input_, *output_); }
    void applyOutput() override;
    void passOutputToInput() override { output_->pass_to_input(*input_); }
    void applyDeltaTime(double dt) override { otg_->delta_time = dt; }

public:
    MoveAbsJ() : dof_(0)
    {
        std::strcpy(nodeName_, "MoveAbsJ");
    }
};

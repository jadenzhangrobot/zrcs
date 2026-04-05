/*
 * @Description: 多轴回零命令
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/TrajectoryCmd.h"
#include "system/nodeFactory.h"
#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class Movehome : public TrajectoryCmd
{
private:
    std::unique_ptr<Ruckig<DynamicDOFs>> otg_;
    std::unique_ptr<InputParameter<DynamicDOFs>> input_;
    std::unique_ptr<OutputParameter<DynamicDOFs>> output_;
    int dof_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override { return otg_->update(*input_, *output_); }
    void applyOutput() override;
    void passOutputToInput() override { output_->pass_to_input(*input_); }
    void applyDeltaTime(double dt) override { otg_->delta_time = dt; }

public:
    Movehome() : dof_(0)
    {
        std::strcpy(nodeName_, "Movehome");
    }
};

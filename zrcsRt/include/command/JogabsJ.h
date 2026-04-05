/*
 * @Description: 关节运动绝对位置指令
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/TrajectoryCmd.h"
#include "system/nodeFactory.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class JogabsJ : public TrajectoryCmd
{
private:
    Ruckig<1> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;
    int axisId_;
    double position_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override { return otg_.update(input_, output_); }
    void applyOutput() override { controller_->axiss[axisId_]->setAxisPositionCmd(output_.new_position[0]); }
    void passOutputToInput() override { output_.pass_to_input(input_); }
    void applyDeltaTime(double dt) override { otg_.delta_time = dt; }

public:
    JogabsJ() : otg_(cycletime * 0.001)
    {
        std::strcpy(nodeName_, "JogabsJ");
    }
};

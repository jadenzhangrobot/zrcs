/*
 * @Description: 关节运动相对位置指令
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/TrajectoryCmd.h"
#include "system/nodeFactory.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class JogJ : public TrajectoryCmd
{
private:
    Ruckig<1> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;
    int axisId_;
    double position_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override;
    void applyOutput() override;
    void passOutputToInput() override;
    void applyDeltaTime(double dt) override;

public:
    JogJ();
};

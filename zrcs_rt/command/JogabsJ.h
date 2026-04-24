/*
 * @Description: 关节运动绝对位置指令
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class JogabsJ : public TrajectoryCmd
{
public:
    

private:
    Ruckig<1> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;
    int axisId_;
    double position_;

protected:
    bool initTrajectory() override;

    void applyOutput() override;
    void applyDeltaTime(double dt) override;

public:
    JogabsJ();
};

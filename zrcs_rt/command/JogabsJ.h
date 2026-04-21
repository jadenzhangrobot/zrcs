/*
 * @Description: 关节运动绝对位置指令
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class JogabsJ : public TrajectoryCmd
{
public:
    CMD_DEFINE(19, PARAM(AxisId) PARAM(TargetPosition))

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
    JogabsJ();
};


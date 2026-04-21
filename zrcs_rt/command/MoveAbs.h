/*
 * @Description: 通用单轴绝对定位（带可选速度/加速度/加加速度参数）
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveAbs : public TrajectoryCmd
{
public:
    CMD_DEFINE(21, PARAM(AxisId) PARAM(Position) PARAM(Vel) PARAM(Acc) PARAM(Jerk))

private:
    Ruckig<1> otg_;
    InputParameter<1> input_;
    OutputParameter<1> output_;
    int axisId_;

protected:
    bool initTrajectory() override;
    Result updateTrajectory() override;
    void applyOutput() override;
    void passOutputToInput() override;
    void applyDeltaTime(double dt) override;

public:
    MoveAbs();
};


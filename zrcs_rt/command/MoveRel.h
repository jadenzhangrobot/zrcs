/*
 * @Description: 通用单轴相对定位（带可选速度/加速度/加加速度参数）
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include <ruckig/ruckig.hpp>

CMD_DEFINE(MoveRel, 22,
    PARAM(AxisId)
    PARAM(Distance)
    PARAM(Vel)
    PARAM(Acc)
    PARAM(Jerk)
)

using namespace ruckig;

class MoveRel : public TrajectoryCmd
{
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
    MoveRel();
};

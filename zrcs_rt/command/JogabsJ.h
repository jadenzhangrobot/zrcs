/*
 * @Description: 关节运动绝对位置指令
 */
#pragma once
#include "system/node/TrajectoryCmd.h"
#include "system/node/NodeFactory.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class JogabsJ : public zrcsSystem::TrajectoryCmd
{
private:
    int axisId_;
    double position_;

protected:
    bool initTrajectory() override;

    void applyOutput() override;

public:
    JogabsJ();
};

/*
 * @Description: 通用单轴绝对定位（带可选速度/加速度/加加速度参数�? */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class MoveAbs : public TrajectoryCmd
{
private:
    int axisId_;

protected:
    bool initTrajectory() override;
  
    void applyOutput() override;


public:
    MoveAbs();
};

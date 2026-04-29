/*
 * @Description: 关节运动相对位置指令
 */
#pragma once
#include "system/base/TrajectoryCmd.h"
#include "system/NodeFactory.h"
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class JogJ : public TrajectoryCmd
{
public:
   

private:
    int axisId_;
    double position_;

protected:
    bool initTrajectory() override;
  
    void applyOutput() override;
    

public:
    JogJ();
};

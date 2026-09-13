/*
 * @Description: 多轴回零命令
 */
#pragma once
#include "system/node/TrajectoryCmd.h"
#include "system/node/NodeFactory.h"
#include <memory>
#include <ruckig/ruckig.hpp>

using namespace ruckig;

class Movehome : public zrcsSystem::TrajectoryCmd
{

private:
    int dof_;
    bool prepared_{false};

protected:
    bool initTrajectory() override;
   
    bool applyOutput() override;
  

public:
    Movehome();
    bool prepare() override;
};

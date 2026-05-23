/**
 * @Description: 轨迹规划基类实现
 */
#include "system/node/TrajectoryCmd.h"
#include "ruckig/result.hpp"
#include <Eigen/Core>

namespace zrcsSystem 
{
TrajectoryCmd::TrajectoryCmd() : baseDeltaTime_(cycletime * 0.001) {}

void TrajectoryCmd::applyDeltaTime(double dt)
{
    otg_->delta_time = dt;
}

void TrajectoryCmd::updateOverride()
{
    double override = shm()->overrideRatio.load(std::memory_order_acquire);
    applyDeltaTime(baseDeltaTime_ * override);
}

Result TrajectoryCmd::updateTrajectory()
{
    return otg_->update(*input_, *output_);
}

void TrajectoryCmd::passOutputToInput()
{
    output_->pass_to_input(*input_);
}

bool TrajectoryCmd::init()
{
    if (!initTrajectory())
    {
        return false;
    }
    updateOverride();
    return true;
}

RunResult TrajectoryCmd::run()
{
    updateOverride();
    auto result = updateTrajectory();  
    applyOutput();
    passOutputToInput();
    if (result == Result::Finished)
    {
        applyOutput();
        passOutputToInput();
        return runResult_= RunResult::SUCCESS;    
    }
    if(result<-1)
    {
        ERROR_PRINT("%s: 轨迹规划失败, result=%d\n", nodeName_, static_cast<int>(result));
        return runResult_= RunResult::FAILED;
    }
    return runResult_= RunResult::EXECUTING;
}

 bool TrajectoryCmd::exit() { return true; }
}
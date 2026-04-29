/**
 * @Description: 轨迹规划基类实现
 */
#include "system/base/TrajectoryCmd.h"

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

void TrajectoryCmd::runStandard()
{
    updateOverride();

    auto result = updateTrajectory();
    if (result == Result::Working)
    {
        applyOutput();
        passOutputToInput();
    }
    else if (result == Result::Finished)
    {
        applyOutput();
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    else
    {
        ERROR_PRINT("%s: 轨迹规划失败\n", nodeName_);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}
Result TrajectoryCmd::updateTrajectory()
{
    return otg_->update(*input_, *output_);
}

void TrajectoryCmd::passOutputToInput()
{
    output_->pass_to_input(*input_);
}

void TrajectoryCmd::init()
{
    if (!initTrajectory())
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    updateOverride();
}

void TrajectoryCmd::run()
{
    runStandard();
}

void TrajectoryCmd::exit() {}

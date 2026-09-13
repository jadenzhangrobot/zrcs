#pragma once

#include "system/node/TrajectoryCmd.h"
#include "system/node/NodeFactory.h"

#include <vector>

/**
 * @brief Continuous joint-speed command planned with Ruckig's velocity API.
 *
 * Command arguments are encoded as [count, v1, ..., vN], where N is the
 * active model DOF. A non-zero target keeps running until the scheduler is
 * stopped. A zero target decelerates all axes and then completes.
 */
class MoveV : public zrcsSystem::TrajectoryCmd
{
private:
    int dof_{0};
    bool prepared_{false};
    bool zeroTarget_{false};
    std::vector<int> axisIds_;

protected:
    bool initTrajectory() override;
    bool applyOutput() override;

public:
    MoveV();

    bool prepare() override;
    zrcsSystem::RunResult run() override;
};

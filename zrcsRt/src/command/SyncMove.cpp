/*
 * @Description: 电子齿轮（固高 GT_SetGearMode / 正运动 CONNECT）
 *               从轴跟随主轴，OutputNode 持续运行
 */
#include "command/SyncMove.h"

void SyncMove::init()
{
    mainAxisId_ = static_cast<int>(command_->args[SyncMoveMainAxis]);
    slaveAxisId_ = static_cast<int>(command_->args[SyncMoveSlaveAxis]);
    ratio_ = command_->args[SyncMoveRatio];

    if (mainAxisId_ < 0 || mainAxisId_ >= static_cast<int>(controller_->axiss.size()) ||
        slaveAxisId_ < 0 || slaveAxisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
}

void SyncMove::run(void)
{
    double masterPos = controller_->axiss[mainAxisId_]->actualPos();
    controller_->axiss[slaveAxisId_]->setAxisPositionCmd(masterPos * ratio_);
}

REGISTERCMD(SyncMove);

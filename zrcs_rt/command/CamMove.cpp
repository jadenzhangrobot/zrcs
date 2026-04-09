/*
 * @Description: 电子凸轮（固高 GT_SetCamMode / 正运动 CAMBOX）
 *               从轴按凸轮表映射主轴位置，OutputNode 持续运行
 */
#include "command/CamMove.h"

void CamMove::init()
{
    mainAxisId_ = static_cast<int>(command_->args[CamMoveMainAxis]);
    slaveAxisId_ = static_cast<int>(command_->args[CamMoveSlaveAxis]);
    tableId_ = static_cast<int>(command_->args[CamMoveTableId]);

    if (mainAxisId_ < 0 || mainAxisId_ >= static_cast<int>(controller_->axiss.size()) ||
        slaveAxisId_ < 0 || slaveAxisId_ >= static_cast<int>(controller_->axiss.size()))
    {
        ERROR_PRINT("CamMove: 轴索引 %d 超出范围\n", mainAxisId_);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    // TODO: load cam table from configuration by tableId_
    // placeholder: linear 1:1 mapping
    camTable_.push_back({0.0, 0.0});
    camTable_.push_back({360.0, 360.0});
}

void CamMove::run(void)
{
    double masterPos = controller_->axiss[mainAxisId_]->actualPos();
    double slaveCmd = interpolate(masterPos);
    controller_->axiss[slaveAxisId_]->setAxisPositionCmd(slaveCmd);
}

REGISTERCMD(CamMove);

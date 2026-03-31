/*
 * @Description: 激活机械单元（使能一组关联轴）
 */
#include "command/ActUnit.h"

void ActUnit::init()
{
    unitId_ = static_cast<int>(command_->args[ActUnitId]);
}

void ActUnit::run(void)
{
    // unitId 映射到轴：简化实现为使能对应轴
    if (controller_->axiss.size() > unitId_)
    {
        controller_->axiss[unitId_]->setModeOfOperation();
        controller_->axiss[unitId_]->setAxisPositionCmd(
            controller_->axiss[unitId_]->actualPos());
        if (controller_->axiss[unitId_]->powerOn())
        {
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        }
        else
        {
            ERROR_PRINT("ActUnit: 轴 %d 操作失败\n", unitId_);
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        }
    }
    else
    {
        ERROR_PRINT("ActUnit: 轴索引 %d 超出范围(max=%zu)\n", unitId_, controller_->axiss.size());
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void ActUnit::exit(void) {}

REGISTERCMD(ActUnit);

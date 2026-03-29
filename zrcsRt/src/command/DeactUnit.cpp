/*
 * @Description: 停用机械单元（失能一组关联轴）
 */
#include "command/DeactUnit.h"

void DeactUnit::init()
{
    unitId_ = static_cast<int>(command_->args[DeactUnitId]);
}

void DeactUnit::run(void)
{
    if (controller_->axiss.size() > unitId_)
    {
        if (controller_->axiss[unitId_]->powerOff())
        {
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        }
        else
        {
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        }
    }
    else
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void DeactUnit::exit(void) {}

REGISTERCMD(DeactUnit);

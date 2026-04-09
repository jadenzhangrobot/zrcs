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
            ERROR_PRINT("DeactUnit: 轴 %d 操作失败\n", unitId_);
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        }
    }
    else
    {
        ERROR_PRINT("DeactUnit: 轴索引 %d 超出范围(max=%zu)\n", unitId_, controller_->axiss.size());
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void DeactUnit::exit(void) {}

REGISTERCMD(DeactUnit);

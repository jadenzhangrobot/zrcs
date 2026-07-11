/**
 * @file    Reset.cpp
 * @brief   复位错误命令实现
 * @author  zhangyongjing
 * @date    2023-03-28
 */

#include "command/Reset.h"
#include "system/log/RtLog.h"

bool Reset::init()
{
    axisId_ = static_cast<int>(command_->args[static_cast<size_t>(ResetArg::AxisId)]);
    return true;
}

zrcsSystem::RunResult Reset::run()
{
    const int axisCount = static_cast<int>(controller_->axes_.size());

    auto resetOne = [this](int id) -> bool {
        if (!controller_->axes_[id]->resetError())
        {
            ERROR_PRINT("Reset: 轴%d 操作失败\n", id);
            return false;
        }
        // resetError() 内部已对齐命令位置；这里再显式同步一次，保持接口语义清晰。
        controller_->axes_[id]->setAxisPositionCmd(
            controller_->axes_[id]->actualPos());
        controller_->axes_[id]->syncCmdHistory();
        INFO_PRINT("Reset: 轴%d 操作成功\n", id);
        return true;
    };

    if (axisId_ >= 0 && axisId_ < axisCount)
    {
        return resetOne(axisId_) ? zrcsSystem::RunResult::SUCCESS
                                 : zrcsSystem::RunResult::FAILED;
    }

    // axisId == 轴总数：复位全部轴（禁止中途成功就 return）
    if (axisId_ == axisCount)
    {
        bool anyFail = false;
        for (int i = 0; i < axisCount; ++i)
        {
            if (!resetOne(i))
            {
                anyFail = true;
            }
        }
        return anyFail ? zrcsSystem::RunResult::FAILED
                       : zrcsSystem::RunResult::SUCCESS;
    }

    ERROR_PRINT("Reset: 轴索引%d 超出范围(max=%d)\n", axisId_, axisCount);
    return zrcsSystem::RunResult::FAILED;
}

bool Reset::exit()
{
    return true;
}

CMD_REGISTER(Reset);

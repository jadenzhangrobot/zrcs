/*
 * @Description: 等待条件满足（通用条件等待）
 */
#include "command/WaitUntil.h"

void WaitUntil::init()
{
    condType_ = static_cast<int>(command_->args[WaitUntilCondType]);
    param1_ = command_->args[WaitUntilParam1];
    param2_ = command_->args[WaitUntilParam2];
    timeoutMs_ = command_->args[WaitUntilTimeoutMs];
    startCount_ = nodeCount_;
}

void WaitUntil::run(void)
{
    bool conditionMet = false;

    switch (condType_)
    {
        case 0:  // 轴到位: param1=轴号, param2=目标位置（允许0.01误差）
        {
            int axisId = static_cast<int>(param1_);
            if (axisId >= 0 && axisId < static_cast<int>(controller_->axiss.size()))
            {
                double pos = controller_->axiss[axisId]->actualPos();
                conditionMet = std::abs(pos - param2_) < 0.01;
            }
            break;
        }
        case 1:  // IO条件: param1=模块索引, param2=位索引
        {
            int modIdx = static_cast<int>(param1_);
            int bitPos = static_cast<int>(param2_);
            if (modIdx >= 0 && modIdx < static_cast<int>(controller_->ios_.size()))
            {
                conditionMet = controller_->ios_[modIdx]->ioRead32(modIdx, bitPos);
            }
            break;
        }
        default:
            ERROR_PRINT("WaitUntil: 未知条件类型 %d\n", condType_);
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            return;
    }

    if (conditionMet)
    {
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        return;
    }

    if (timeoutMs_ > 0)
    {
        double elapsedMs = (nodeCount_ - startCount_) * cycletime;
        if (elapsedMs >= timeoutMs_)
        {
            ERROR_PRINT("WaitUntil: 等待超时\n");
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            return;
        }
    }
    nodeCount_++;
}

void WaitUntil::exit(void) {}

REGISTERCMD(WaitUntil);

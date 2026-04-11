/*
 * @Description: 位置锁存（固高 GT_SetCapture / 正运动 REGIST）
 *               硬件IO触发时记录精确编码器位置
 */
#include "command/PosCapture.h"

void PosCapture::init()
{
    axisId_ = static_cast<int>(command_->args[PosCaptureAxisId]);
    ioIndex_ = static_cast<int>(command_->args[PosCaptureIOIndex]);
    bitPos_ = static_cast<int>(command_->args[PosCaptureBitPos]);
    edge_ = static_cast<int>(command_->args[PosCaptureEdge]);

    if (axisId_ < 0 || axisId_ >= static_cast<int>(controller_->axiss.size()) ||
        ioIndex_ < 0 || ioIndex_ >= static_cast<int>(controller_->ios_.size()))
    {
        ERROR_PRINT("PosCapture: 轴索引 %d 超出范围\n", axisId_);
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    lastIoState_ = controller_->ios_[ioIndex_]->ioRead32(ioIndex_, bitPos_);
    shm()->captureTriggered.store(false, std::memory_order_release);
}

void PosCapture::run(void)
{
    bool currentState = controller_->ios_[ioIndex_]->ioRead32(ioIndex_, bitPos_);

    bool triggered = false;
    if (edge_ == 0 && !lastIoState_ && currentState)  // rising edge
    {
        triggered = true;
    }
    else if (edge_ == 1 && lastIoState_ && !currentState)  // falling edge
    {
        triggered = true;
    }

    if (triggered)
    {
        double pos = controller_->axiss[axisId_]->actualPos();
        zrcs::CaptureData cap{};
        cap.pos[axisId_] = pos;
        zrcs::lfl_write(shm()->captureResult, cap);
        shm()->captureTriggered.store(true, std::memory_order_release);
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        return;
    }

    lastIoState_ = currentState;
}

void PosCapture::exit(void) {}

REGISTERCMD(PosCapture);

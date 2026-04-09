/*
 * @Description: 位置锁存（固高 GT_SetCapture / 正运动 REGIST）
 *               硬件IO触发时记录精确编码器位置
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class PosCapture : public zrcsSystem::CmdNode
{
private:
    int axisId_;
    int ioIndex_;
    int bitPos_;
    int edge_;  // 0=上升沿, 1=下降沿
    bool lastIoState_;

public:
    PosCapture() : axisId_(0), ioIndex_(0), bitPos_(0), edge_(0), lastIoState_(false)
    {
        std::strcpy(nodeName_, "PosCapture");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

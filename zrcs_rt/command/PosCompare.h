/*
 * @Description: 位置比较输出（固高 GT_SetCompare / 正运动 HW_PSWITCH）
 *               当轴到达指定位置时自动触发IO输出
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include <cmath>

class PosCompare : public zrcsSystem::CmdNode
{
private:
    int axisId_;
    double targetPos_;
    int ioModule_;
    int bitPos_;
    bool ioValue_;
    bool triggered_;

public:
    PosCompare() : axisId_(0), targetPos_(0), ioModule_(0), bitPos_(0),
                   ioValue_(false), triggered_(false)
    {
        std::strcpy(nodeName_, "PosCompare");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

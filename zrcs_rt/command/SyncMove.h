/*
 * @Description: 电子齿轮（固高 GT_SetGearMode / 正运动 CONNECT）
 *               从轴跟随主轴，OutputNode 持续运行
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class SyncMove : public zrcsSystem::CmdNode
{
private:
    int mainAxisId_;
    int slaveAxisId_;
    double ratio_;

public:
    SyncMove() : mainAxisId_(0), slaveAxisId_(0), ratio_(1.0)
    {
        std::strcpy(nodeName_, "SyncMove");
    }

    void init() override;
    void run(void) override;
    void exit(void) override {}
};

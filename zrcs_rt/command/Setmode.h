/*
 * @Description: 设置伺服控制模式命令
 */
#pragma once
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"

class Setmode : public zrcsSystem::CmdNode
{
public:
    CMD_DEFINE(6, PARAM(AxisId) PARAM(Mode))

    Setmode()
    {
        std::strcpy(nodeName_, "Setmode");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;

private:
    int axisId_;
    int mode_;
};


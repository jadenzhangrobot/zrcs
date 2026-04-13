/*
 * @Description: 设置伺服控制模式命令
 */
#pragma once
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"

CMD_DEFINE(Setmode, 6,
    PARAM(AxisId)
    PARAM(Mode)
)

class Setmode : public zrcsSystem::CmdNode
{
private:
    int axisId_;
    int mode_;

public:
    Setmode()
    {
        std::strcpy(nodeName_, "Setmode");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

/*
 * @Description: 设置伺服控制模式命令
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

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

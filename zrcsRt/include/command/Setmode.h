/*
 * @Description: 设置伺服控制模式命令
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"

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

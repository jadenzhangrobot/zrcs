/*
 * @Description: 受控停止命令
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"

class Stop : public zrcsSystem::CmdNode
{
private:
    int axisId_;

public:
    Stop()
    {
        std::strcpy(nodeName_, "Stop");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

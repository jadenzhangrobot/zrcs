/*
 * @Description: 设置全局速度倍率 (0.0 ~ 1.0)
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"

class SetOverride : public zrcsSystem::CmdNode
{
private:
    double value_;

public:
    SetOverride()
    {
        std::strcpy(nodeName_, "SetOverride");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

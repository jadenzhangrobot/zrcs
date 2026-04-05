/*
 * @Description: 动态设置软限位（固高 GT_SetSoftLimit）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"

class SetPosLimit : public zrcsSystem::CmdNode
{
public:
    SetPosLimit()
    {
        std::strcpy(nodeName_, "SetPosLimit");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

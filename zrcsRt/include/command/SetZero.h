/*
 * @Description: 设置当前位置为零点
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"

class SetZero : public zrcsSystem::CmdNode
{
private:
    int axisId_;

public:
    SetZero()
    {
        std::strcpy(nodeName_, "SetZero");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

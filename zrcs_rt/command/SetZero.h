/*
 * @Description: 设置当前位置为零点
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"

class SetZero : public zrcsSystem::CmdNode
{
public:
    CMD_DEFINE(10, PARAM(AxisId))

    SetZero()
    {
        std::strcpy(nodeName_, "SetZero");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;

private:
    int axisId_;
};
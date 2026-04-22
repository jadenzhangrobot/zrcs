/*
 * @Description: 设置当前位置为零�? */
#pragma once
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class SetZero : public zrcsSystem::CmdNode
{
public:
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

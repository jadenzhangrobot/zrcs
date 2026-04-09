/*
 * @Description: 设置组输出（多bit整数）
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/nodeFactory.h"

class SetGO : public zrcsSystem::CmdNode
{
private:
    int moduleIndex_;
    int value_;

public:
    SetGO()
    {
        std::strcpy(nodeName_, "SetGO");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

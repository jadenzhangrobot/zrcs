/*
 * @Description: 奇异区域处理策略（ABB SingArea）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/basenodeInterface.h"
#include "system/nodeFactory.h"

class SingArea : public zrcsSystem::CmdNode
{
public:
    SingArea()
    {
        std::strcpy(nodeName_, "SingArea");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

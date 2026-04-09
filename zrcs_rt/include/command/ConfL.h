/*
 * @Description: 直线运动配置监控开关（ABB ConfL）
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/nodeFactory.h"

class ConfL : public zrcsSystem::CmdNode
{
public:
    ConfL()
    {
        std::strcpy(nodeName_, "ConfL");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

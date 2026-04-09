/*
 * @Description: 关节配置监控开关（ABB ConfJ）
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/nodeFactory.h"

class ConfJ : public zrcsSystem::CmdNode
{
public:
    ConfJ()
    {
        std::strcpy(nodeName_, "ConfJ");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

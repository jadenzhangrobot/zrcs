/*
 * @Description: 读取数字/模拟输入
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/nodeFactory.h"

class IORead : public zrcsSystem::CmdNode
{
private:
    int moduleIndex_;
    int bitPos_;
    int type_;  // 0=数字, 1=模拟

public:
    IORead()
    {
        std::strcpy(nodeName_, "IORead");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

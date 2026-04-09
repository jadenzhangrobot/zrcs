/*
 * @Description: 等待数字输入信号达到指定值
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class WaitDI : public zrcsSystem::CmdNode
{
private:
    int moduleIndex_;
    int bitPos_;
    bool targetValue_;
    double timeoutMs_;
    uint64_t startCount_;

public:
    WaitDI()
    {
        std::strcpy(nodeName_, "WaitDI");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

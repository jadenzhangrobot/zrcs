/*
 * @Description: 脉冲数字输出（输出一段时间后自动复位）
 */
#pragma once
#include "config/cmdArgs.h"
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"

class PulseDO : public zrcsSystem::CmdNode
{
private:
    int moduleIndex_;
    int bitPos_;
    double pulseTimeMs_;
    uint64_t startCount_;

public:
    PulseDO()
    {
        std::strcpy(nodeName_, "PulseDO");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

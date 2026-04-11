/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机使能指令
 */
#pragma once

#include "config/CmdArgs.h"
#include "shared_memory/ShmLayout.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"

class Enable : public zrcsSystem::CmdNode
{
private:
    int axisId_;

public:
    Enable()
    {
        std::strcpy(nodeName_, "Enable");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};


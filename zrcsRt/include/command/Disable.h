/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机失能指令
 */
#pragma once
#include "system/base/basenodeInterface.h"
#include "system/nodeFactory.h"

class Disable : public zrcsSystem::CmdNode
{
private:
    int axisId_;

public:
    Disable()
    {
        std::strcpy(nodeName_, "Disable");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};

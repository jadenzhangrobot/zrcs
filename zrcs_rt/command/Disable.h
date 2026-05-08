/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 电机失能指令
 */
#pragma once
#include "system/node/BaseNodeInterface.h"
#include "system/node/NodeFactory.h"

class Disable : public zrcsSystem::CmdNode
{
public:


    Disable()
    {
        std::strcpy(nodeName_, "Disable");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;

private:
    int axisId_;
};


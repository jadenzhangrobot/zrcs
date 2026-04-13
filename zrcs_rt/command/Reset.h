/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 15:25:04
 * @LastEditTime: 2023-06-10 15:12:05
 * @Description: 复位错误指令
 */
#pragma once

#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include "system/CmdMeta.h"
#include <iostream>

CMD_DEFINE(Reset, 3,
    PARAM(AxisId)
)

class Reset : public zrcsSystem::CmdNode
{
private:
    int axisId_;

public:
    Reset()
    {
        std::strcpy(nodeName_, "Reset");
    }

    void init() override;
    void run(void) override;
    void exit(void) override;
};


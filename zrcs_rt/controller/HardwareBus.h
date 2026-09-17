/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: hardware bus abstraction
 */
#pragma once

#include "shared_memory/ShmLayout.h"

namespace ZrcsHardware {

/// 硬件总线抽象（EtherCAT / MuJoCo 等过程数据的周期收发）。
class HardwareBus
{
public:
    virtual ~HardwareBus();
    virtual void bindSharedBlock(zrcs::SharedBlock* block);
    virtual void send() = 0;
    virtual void receive() = 0;
};

} // namespace ZrcsHardware

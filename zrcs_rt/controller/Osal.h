/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: os abstraction layer (osal)
 */
#pragma once

#include <cstdint>
#include <functional>

// ── 全局作用域枚举（与全局头文件历史一致，全工程无前缀引用）──

/* Network error code */
typedef enum {
    mcNetworkGood = 0,
    mcNetworkConnectionBreak = 1,
} MC_NETWORK_ERROR_CODE;

namespace ZrcsHardware {

/// RTOS 抽象接口：实时任务、周期定时与共享内存读取。
class Rtos
{
public:
    virtual ~Rtos();
    virtual void real_task(std::function<void()> strategy) = 0;
    virtual void rtos_task_create(void) = 0;
    virtual void rtos_task_join(void) = 0;
    virtual void rtos_task_stop();
    virtual void rtos_set_periodic(int period) = 0;
    virtual std::uint64_t rtos_timer_read(void);
};

} // namespace ZrcsHardware

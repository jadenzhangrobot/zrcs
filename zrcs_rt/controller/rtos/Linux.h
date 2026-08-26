/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-24 18:47:51
 * @Description: 非实时平台（Windows / 普通 Linux）控制循环
 *
 * simulation: 20 ms 墙钟批次内推进多个 cycletime 仿真步。
 * standard: 墙钟周期与 cycletime 逻辑步长 1:1，落后时有限追赶。
 * 每个仿真子步反馈均带逻辑时间戳，由 NRT 批量发布。
 */

#pragma once

#include "controller/ControllerInterface.h"

#include <atomic>
#include <functional>
#include <thread>

namespace ZrcsHardware {

class Nativelinux : public Rtos
{
public:
    std::thread my_thread;
    std::function<void()> strategy_{nullptr};
    std::atomic<bool> running_{true};

    void rtos_task_create(void) override;
    void real_task(std::function<void()> strategy) override
    {
        strategy_ = strategy;
    }
    void rtos_task_join(void) override
    {
        my_thread.join();
    }

    void rtos_task_stop() override;

    void rtos_set_periodic(int /*period*/) override
    {
    }
};

} // namespace ZrcsHardware

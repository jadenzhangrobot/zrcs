/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-24 18:47:51
 * @LastEditTime: 2023-08-10 08:49:02
 * @Description: linux系统线程封装
 * 
 */

#pragma once

#include "controller/ControllerInterface.h"
#include <cstdint>
#include <thread>
#include <chrono>
#include <functional>
namespace ZrcsHardware {

class Nativelinux : public Rtos
{
    public:
    std::thread my_thread;
    std::function<void()> strategy_{ nullptr };
    std::atomic<bool> running_{true};

    void rtos_task_create(void)override;
    void real_task(std::function<void()> strategy) override
    {
        strategy_=strategy;
    }
    void rtos_task_join(void) override
    {
       my_thread.join();
    }

    void rtos_task_stop() override;

    void rtos_set_perioic(int perioic) override
    {

    }

    
    // std::uint64_t rtos_timer_read(void)override
    // {
        
    // }
};

}



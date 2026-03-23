/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-24 18:47:51
 * @LastEditTime: 2023-08-10 08:49:02
 * @Description: linux系统线程封装
 * 
 */

#ifndef linux_H
#define linux_H
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

    void rtos_task_create(void)override
    {

        my_thread=std::thread([this](void)
        {
            using clock = std::chrono::high_resolution_clock;
            const std::chrono::milliseconds target_period(10); // 目标周期10ms
            auto next_wake_time = clock::now();
            while(running_.load(std::memory_order_relaxed))
            {
                next_wake_time += target_period;
                if (strategy_!=nullptr) {
                      strategy_();
                }
                  auto sleep_until = next_wake_time;
              std::this_thread::sleep_until(sleep_until);
            }
        });
    }
    void real_task(std::function<void()> strategy) override
    {
        strategy_=strategy;
    }
    void rtos_task_join(void) override
    {
       my_thread.join();
    }

    void rtos_task_stop() override
    {
        running_.store(false, std::memory_order_release);
        if (my_thread.joinable()) {
            my_thread.join();
        }
    }

    void rtos_set_perioic(int perioic) override
    {

    }

    
    // std::uint64_t rtos_timer_read(void)override
    // {
        
    // }
};

}


#endif
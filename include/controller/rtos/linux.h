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
#include <pthread.h>
#include <thread>
namespace ZrcsHardware {

class Nativelinux:Rtos
{
    public:
    std::thread my_thread;
    std::uint32_t control_period=1;
    std::function<void()> strategy_{ nullptr };
    void rtos_task_create(void)override
    {
        my_thread=std::thread([this](void)
        {
            while(1)
            {  
                if (strategy_!=nullptr) {
                      strategy_();
                }               
                std::this_thread::sleep_for(std::chrono::milliseconds(control_period));  
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

    void rtos_set_perioic(int perioic) override
    {
      
    } 	

    
    // std::uint64_t rtos_timer_read(void)override
    // {
        
    // }
};

}


#endif
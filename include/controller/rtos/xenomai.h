/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-24 18:48:02
 * @LastEditTime: 2023-08-03 21:59:25
 * @Description: xenomai实时线程的封装
 * 
 */
#ifndef XENOMAI_H
#define XENOMAI_H

#include "controller/controller_interface.h"
#include "controller/motor/ethercat/EthercatMaster.h"
#include <alchemy/timer.h> 
#include <alchemy/task.h> 
#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
namespace controller 
{
 class xenomai:Rtos
{
   public:
    RT_TASK task_desc;
    std::function<void()> strategy_{ nullptr };
    ~xenomai(){

     rtos_task_join();

    }
    static void real_fun(void* arg)
    {       
     
      xenomai* p=(xenomai*)arg;
       int err = rt_task_set_periodic(NULL,TM_NOW, 1000000);   
        while (true) 
        {
            if (p->strategy_!=nullptr)
            {
               p->strategy_();
            }
           rt_task_wait_period(NULL);
        }       
    }
    void rtos_task_create(void)override
    {

       int err =rt_task_create(&task_desc,"task_desc",0,99,0); 
        if(err<0) 
        { 
          printf("rt_task_create : %s\n", strerror(errno));  

        }
        int cpu_id =7;                // 需要绑定的cpu
            cpu_set_t mask;                // cpu核的位掩码
            CPU_ZERO(&mask);               // 置空
            CPU_SET(cpu_id, &mask);        // 将需要绑定的cpu号设置在mask中 
        int a= rt_task_set_affinity(&task_desc,&mask); 
        err=rt_task_start(&task_desc,real_fun,(void*)this); 

        if(err<0) 
        { 

            printf("rt_task_start : %s\n", strerror(errno));   
        }
     }
  
      
    void rtos_task_join(void) override
    {
        rt_task_join(&task_desc); 	
    }
    void rtos_set_perioic(int period) override
    {
        rt_task_set_periodic(&task_desc,TM_NOW,period);
    } 	

    
     std::uint64_t rtos_timer_read(void) override
    {
        RTIME time=rt_timer_read();
        return time;
    }
     void real_task(std::function<void()> strategy) override
    {
        strategy_=strategy;
    }

 };


}


#endif
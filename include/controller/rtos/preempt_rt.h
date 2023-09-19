#ifndef PREEMPT_RT_H
#define PREEMPT_RT_H
#include "controller/controller_interface.h"
#include <bits/types/struct_timespec.h>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <thread>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

namespace controller {
 
class Preempt_rt:Rtos
{
    public:
    struct sched_param param;
    pthread_attr_t attr;
    pthread_t thread;
    int ret;
    struct period_info
      {
        struct timespec next_period;
        long period_ns;
    };
    // std::thread my_thread;
    // std::uint32_t control_period=1;
    ~Preempt_rt()
    {
       

    }
   std::function<void()> strategy_{ nullptr };
static void inc_period(struct period_info *pinfo) 
{
        pinfo->next_period.tv_nsec += pinfo->period_ns;
 
        while (pinfo->next_period.tv_nsec >= 1000000000) {
                /* timespec nsec overflow */
                pinfo->next_period.tv_sec++;
                pinfo->next_period.tv_nsec -= 1000000000;
        }
}
 
static void periodic_task_init(struct period_info *pinfo)
{
        /* for simplicity, hardcoding a 1ms period */
        pinfo->period_ns = 1000000;
 
        clock_gettime(CLOCK_MONOTONIC, &(pinfo->next_period));
}
 
static void wait_rest_of_period(struct period_info *pinfo)
{
        inc_period(pinfo);
 
        /* for simplicity, ignoring possibilities of signal wakes */
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &pinfo->next_period, NULL);
}
   static void* real_fun(void* arg)
    {
        struct period_info pinfo;
 
        periodic_task_init(&pinfo);
        Preempt_rt* p=(Preempt_rt*)arg;
        while (true) 
        {
            if (p->strategy_!=nullptr)
            {
               p->strategy_();
            }
           std::cout<<"1111"<<std::endl;
             wait_rest_of_period(&pinfo);
        }
       
       return nullptr;
    }
    void rtos_task_create(void)override
    {
       if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) 
       {
              // LOGGER_INFO("glrbus socket error");
              // LOGGER_INFO(strerror(errno));
               
        }
 
        /* Initialize pthread attributes (default values) */
        ret = pthread_attr_init(&attr);
        if (ret) {
               // LOGGER_INFO("pthread_attr_init error");
               // LOGGER_INFO(strerror(errno));
        }
 
        /* Set a specific stack size  */
        ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
        if (ret) {
               // LOGGER_INFO("pthread_attr_setstacksize error");
              //  LOGGER_INFO(strerror(errno));
        }
 
        /* Set scheduler policy and priority of pthread */
        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if (ret) {
               // LOGGER_INFO("pthread_attr_setschedpolicy error");
               // LOGGER_INFO(strerror(errno));
        }
        param.sched_priority =99;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) {
               //  LOGGER_INFO("pthread_attr_setschedparam error");
              //  LOGGER_INFO(strerror(errno));
        }
        /* Use scheduling parameters of attr */
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret) {
               // LOGGER_INFO("pthread_attr_setinheritsched error");
               // LOGGER_INFO(strerror(errno));
        }
 
        /* Create a pthread with specified attributes */
        ret = pthread_create(&thread, &attr,real_fun, (void*)this);
        if (ret) {
              //  LOGGER_INFO("pthread_create error");
              //  LOGGER_INFO(strerror(errno));
        }
    }
    void real_task(std::function<void()> strategy) override
    {
        strategy_=strategy;
    }
    // void rtos_task_join(void) override
    // {
     
               
    // }

    void rtos_set_perioic(int perioic) override
    {
      
    } 	

    void rtos_task_join(void) override
    {
         pthread_join(thread, NULL);
    }


};

}


#endif
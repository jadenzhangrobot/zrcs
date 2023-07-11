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
    // std::thread my_thread;
    // std::uint32_t control_period=1;
    ~Preempt_rt()
    {
        ret = pthread_join(thread, NULL);
        if (ret)
        {
              LOGGER_INFO("pthread_join error");
              LOGGER_INFO(strerror(errno));
        }

    }
   std::function<void()> strategy_{ nullptr };
   
   static void* real_fun(void* arg)
    {
        
        Preempt_rt* p=(Preempt_rt*)arg;
        while (true) 
        {
            if (p->strategy_!=nullptr)
            {
               p->strategy_();
            }
           struct timespec ts;
           struct timespec tt;
             ts.tv_sec=0;
             ts.tv_nsec=100000000;
            clock_nanosleep(CLOCK_REALTIME, 0,&ts, NULL);
            clock_gettime(CLOCK_MONOTONIC,&tt);
            printf("%ld %ld\n",tt.tv_sec,tt.tv_nsec);
        }
       
       return nullptr;
    }
    void rtos_task_create(void)override
    {
       if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) 
       {
               LOGGER_INFO("glrbus socket error");
               LOGGER_INFO(strerror(errno));
               
        }
 
        /* Initialize pthread attributes (default values) */
        ret = pthread_attr_init(&attr);
        if (ret) {
                LOGGER_INFO("pthread_attr_init error");
                LOGGER_INFO(strerror(errno));
        }
 
        /* Set a specific stack size  */
        ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
        if (ret) {
                LOGGER_INFO("pthread_attr_setstacksize error");
                LOGGER_INFO(strerror(errno));
        }
 
        /* Set scheduler policy and priority of pthread */
        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if (ret) {
                LOGGER_INFO("pthread_attr_setschedpolicy error");
                LOGGER_INFO(strerror(errno));
        }
        param.sched_priority =99;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) {
                 LOGGER_INFO("pthread_attr_setschedparam error");
                LOGGER_INFO(strerror(errno));
        }
        /* Use scheduling parameters of attr */
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret) {
                LOGGER_INFO("pthread_attr_setinheritsched error");
                LOGGER_INFO(strerror(errno));
        }
 
        /* Create a pthread with specified attributes */
        ret = pthread_create(&thread, &attr,real_fun, (void*)this);
        if (ret) {
                LOGGER_INFO("pthread_create error");
                LOGGER_INFO(strerror(errno));
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

};

}


#endif
#ifndef ZRCS
#define ZRCS
#include "nodeManager.h"
#include <exception>
#include <iostream>
#include <thread>
#include <string>
#include <future>
#include "common/Shared memory/rt_process.h"


namespace zrcsSystem {
    class Zrcs {
    private:
    std::thread terminal;
    public:
        MotionController *mc=nullptr;
        RTProcess *rtProcess=nullptr;
    
    public:
        Zrcs() :rtProcess(new RTProcess("rtMotion"))
        {   
            mc=new MotionController(rtProcess);
            rtProcess->initialize();
                         
        }
        
        void run(void)
        {
                mc->run();
        }   

    
    ~Zrcs() 
    {
                     
               terminal.join();               
               delete mc;
               delete rtProcess;
        }
    };
} 
#endif
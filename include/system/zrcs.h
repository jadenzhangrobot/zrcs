#ifndef ZRCS
#define ZRCS
#include "nodeManager.h"
#include <thread>
#include "common/Shared memory/rtProcess.h"


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
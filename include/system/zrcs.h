#ifndef ZRCS
#define ZRCS
#include "motionController.h"
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
               
            
            //     terminal = std::thread([this]() 
            //     {
            //     //     while (true) 
            //     //     {   
            //     //         Command cmd;
            //     //         if(rtProcess->shared_block_->command_queue.pop(cmd))
            //     //         {
            //     //             std::string cmd_(cmd.cmd);
            //     //             mc->cmdQueue->writeCmd(cmd_);
            //     //         }

            //     //         //指令字符串
            //     //         // std::string cmd;
            //     //         // std::getline(std::cin, cmd);
            //     //         // ct->cmdQueue->writeCmd(cmd);
            //     //          std::this_thread::sleep_for(std::chrono::milliseconds(100));
            //     //     }
            //  });

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
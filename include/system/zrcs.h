#ifndef ZRCS
#define ZRCS
#include "centre.h"
#include <exception>
#include <iostream>
#include <thread>
#include <string>
#include <future>
#include "rt/rt_process.h"

namespace zrcsSystem {
    class Zrcs {
    private:
    std::thread terminal;
    public:
        Centre *ct;
        RTProcess *rtProcess;
    
    public:
        Zrcs() : ct(new Centre),rtProcess(new RTProcess("rtMotion"))
        {   
            rtProcess->initialize();
                         
        }
        
        void run(void)
        {
               
            
                terminal = std::thread([this]() 
                {
                    while (true) 
                    {   
                        Command cmd;
                        if(!rtProcess->shared_block_->command_queue.pop(cmd))
                        {
                            std::string cmd_(cmd.cmd);
                            ct->cmdQueue->writeCmd(cmd_);
                        }

                        //指令字符串
                        // std::string cmd;
                        // std::getline(std::cin, cmd);
                        // ct->cmdQueue->writeCmd(cmd);
                         std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                });
    }

    
    ~Zrcs() {
                     
               terminal.join();               
               delete ct;
               delete rtProcess;
        }
    };
} 
#endif
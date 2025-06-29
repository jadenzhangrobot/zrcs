#ifndef ZRCS
#define ZRCS
#include "centre.h"
#include "server/cppzmq/zmq.hpp"
#include "server/cppzmq/zmq_addon.hpp"
#include <exception>
#include <iostream>
#include <thread>
#include <string>
#include <future>
#include "statusData.pb.h"

namespace zrcsSystem {
    class Zrcs {
    private:
    std::thread terminal;
    public:
        Centre *ct;
        std::thread serverTh;
    
    public:
        Zrcs() : ct(new Centre)
        {   
                         
        }
        
        void run(void)
        {
                ct->init();
            
                terminal = std::thread([this]() 
                {
                    while (true) 
                    {   
                        //指令字符串
                        std::string cmd;
                        std::getline(std::cin, cmd);
                        ct->cmdQueue->writeCmd(cmd);
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                });
    }

    
    ~Zrcs() {
               serverTh.join();        
               terminal.join();
               delete ct; 
        }
    };
} 
#endif
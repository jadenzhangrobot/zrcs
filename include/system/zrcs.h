// #ifndef ZRCS
// #define ZRCS
// #include "nodeManager.h"
// #include <thread>
// #include "common/sharedMemory/rtProcess.h"


// namespace zrcsSystem {
//     class Zrcs {
//     private:
//     std::thread terminal;
//     public:
//         NodeManger *mc=nullptr;
//         RTProcess *rtProcess=nullptr;
    
//     public:
//         Zrcs() :rtProcess(new RTProcess("rtMotion"))
//         {   
//             mc=new NodeManger(rtProcess);
//             rtProcess->initialize();
                         
//         }
        
//         void run(void)
//         {
//                 mc->run();
//         }   

    
//     ~Zrcs() 
//     {
                     
//                terminal.join();               
//                delete mc;
//                delete rtProcess;
//         }
//     };
// } 
// #endif
#ifndef ZRCS
#define ZRCS
#include "centre.h"
#include "server/opcuaServer/OpcuaServer.h"
namespace zrcsSystem {
    class Zrcs {
    public:
    Centre *ct;
    zrcsServer::OpcuaServer* ops;
    public:
    Zrcs() : ct(new Centre),ops(new zrcsServer::OpcuaServer()) 
    {
   
       
           ct->init();
           ops->OpcuaInit();
        
    }
    void run(void)
    {
         ops->OpcuaRun();
    }

    
    ~Zrcs() { 
        delete ct; 
        
        }
    };
} 
#endif
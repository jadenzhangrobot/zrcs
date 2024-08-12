#ifndef ZRCS
#define ZRCS
#include "centre.h"
#include "OpcuaServer.h"
#include "system/OpcuaServer.h"
namespace zrcs_system {
    class Zrcs {
    public:
    Centre *ct;
    OpcuaServer* OPS;
    public:
    Zrcs() : ct(new Centre) {
         std::unique_ptr<OpcuaServer> OS(new OpcuaServer(ct));
       
         OPS=  OS.release();
        
    }
    void init (void)
    {
        ct->init();
        OPS->OpcuaInit();
    }
    void run(void)
    {
         OPS->OpcuaRun();
    }

    ~Zrcs() { 
        delete ct; 
        
        }
    };
} // namespace zrcs_system
#endif
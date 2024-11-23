#ifndef ZRCS
#define ZRCS
#include "centre.h"
namespace zrcsSystem {
    class Zrcs {
    public:
    Centre *ct;
    public:
    Zrcs() : ct(new Centre)
    {
   
       
           ct->init();
          
        
    }
    void run(void)
    {
        
    }

    
    ~Zrcs() { 
        delete ct; 
        
        }
    };
} 
#endif
#ifndef ZRCSZMQ
#define ZRCSZMQ
#include <zmq.hpp>
#include "zmq_addon.hpp"
namespace Server
{
    class ZrcsZmq
    {
        zmq::context_t* ctx;
        ZrcsZmq():ctx(new zmq::context_t(0))
        {   
             
        }
        pub()
        {



        }

        ~ZrcsZmq()
        {
            delete ctx;
        }
    }
    ;
}
#endif
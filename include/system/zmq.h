/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:zmq.h
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-08
 */
#ifndef ZMQ_H
#define ZMQ_H
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <zmq.h>
#include <zmq.hpp>
#include <iostream>
class Zmq_cmd{
       private:
       void * context;
       void* subscriber;
       void* publisher;
       std::string ip_addr;
        public:
        Zmq_cmd()
        {
            context = zmq_ctx_new ();
            assert(context != NULL);
            subscriber  = zmq_socket (context, ZMQ_SUB);
            if(subscriber == NULL)
             {
                std::cout<< "zmq_ctx_new error" << std::endl;
                //LOGGER_INFO("glrzmq subsocket error");
               // LOGGER_INFO(strerror(errno));
             }        
            int rsub= zmq_connect (subscriber,"tcp://10.16.11.77:5556");
            //int rsub= zmq_connect (subscriber,"ipc:///tmp/MotionCtrlMsgDownAdress");
             if(rsub<0)
             {
                 std::cout<< "zmq_ctx_new error" << std::endl;
                //LOGGER_INFO("glrzmq connect error");
                //LOGGER_INFO(strerror(errno));
             }        

                    // int rc = zmq_bind(subscriber , "tcp://*:5556");
          
            zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

            publisher=zmq_socket (context, ZMQ_PUB);
             if(publisher==NULL)
             {
                std::cout<< "zmq_ctx_new error" << std::endl;
                //LOGGER_INFO("glrzmq pubsocket error");
               // LOGGER_INFO(strerror(errno));
             }    
            int rbind = zmq_bind(publisher , "tcp://*:5555");
            if(rbind<0)
            {   
                std::cout<< "zmq_ctx_new error" << std::endl;
                //LOGGER_INFO("glrzmq bind error");
               // LOGGER_INFO(strerror(errno));
            }
        }
         
     ~Zmq_cmd(){
            zmq_close(subscriber);
            zmq_close(publisher);
            zmq_ctx_destroy(context);
        }
        int sub(char* buffer)
        { 
           int ret= zmq_recv(subscriber, buffer, 128, 0);
           return ret;
        }

         int pub(const void* bull,size_t len, int flag)
        {    
               zmq_msg_t msg;
               zmq_msg_init_size(&msg, len);
               memcpy(zmq_msg_data(&msg), bull, len);
               int sent = zmq_msg_send(&msg, publisher, flag);
               return sent;
        } 
    };
#endif
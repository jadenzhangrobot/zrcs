/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-05-30 21:41:05
 * @LastEditTime: 2023-06-13 10:50:30
 * @Description: 
 * 
 */
#ifndef ZMQ_H
#define ZMQ_H
#include "zmq.hpp"
#include "iostream"
namespace zrcs_system {
    struct status_frame
    {
        uint8_t drive_status[6];
        double position[6];
        int Voltage[6];
        int Velocity[6];
        int current[6];
        uint8_t error[6];
        uint8_t enabled[6];
    };
    struct cmd_frame
    {
        char type[20]; //命令类型
        int status;    //0--表示初始状态
        //1--表示执行指令中
        //2--表示指令执行完毕
        //3--表示指令执行失败
        char error[30];
    };
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
                int rc= zmq_connect (subscriber,"tcp://192.168.195.33:5556");
                assert(subscriber  != NULL);
            // int rc = zmq_bind(subscriber , "tcp://*:5556");
                assert (rc == 0);
                zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

                publisher=zmq_socket (context, ZMQ_PUB);
                assert(publisher != NULL);
                rc = zmq_bind(publisher , "tcp://*:5556");
                if(rc==0)
                {   
                    std::cout<<"zmq_socket error"<<strerror(errno)<<std::endl;
                }
            

            }
            
        ~Zmq_cmd(){
                zmq_close(subscriber);
                zmq_close(publisher);
                zmq_ctx_destroy(context);
            }
            int sub(char* buffer)
            { 
            int ret= zmq_recv(subscriber, buffer, 30, 0);
            return ret;
            }

            int pub(const void* bull,size_t len, int flag)
            {    
                zmq_msg_t msg;
                zmq_msg_init_size(&msg, len);
                memcpy(zmq_msg_data(&msg), bull, len);
                int sent = zmq_msg_send(&msg, publisher, flag);
                //int ret = zmq_send(publisher, bull, sizeof(bull), 0);
                return sent;
            } 
    };
}
#endif
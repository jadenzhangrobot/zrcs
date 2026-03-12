// #ifndef ZRCSSERVER
// #define ZRCSSERVER
// #include <zmq.hpp>
// #include "zmq_addon.hpp"
// #include "server/cppzmq/zmq.hpp"
// #include "server/cppzmq/zmq_addon.hpp"
// #include <iostream>
// #include <thread>
// #include <string>
// #include <future>
// namespace Server
// {
//      class ZrcsServer 
//      {
//         std::thread terminal;
//         private:
//             zmq::context_t* ctx;
//             zmq::socket_t* statuspub;
//         public:
         
       
      
//         public:
//         ZrcsServer() : ctx(new zmq::context_t())
//         {   
            
//                  auto cmdReceive=new zmq::socket_t(*ctx,zmq::socket_type::rep); 
//                  cmdReceive->bind("tcp://*:8887");

//                    statuspub=new zmq::socket_t(*ctx,zmq::socket_type::pub);
//                    statuspub->bind("tcp://*:8888");
                 
//                  auto dataPub=new zmq::socket_t(*ctx,zmq::socket_type::pub);
//                       dataPub->bind("tcp://*:8889");

             
//         }
//         template <typename T>
//         void pubZrcsStatus(const T& protobuf_msg)
//          {
//                      std::string serialized_data;
//                      protobuf_msg.SerializeToString(&serialized_data);                
//                      zmq::message_t zmqMessage(serialized_data.size());
//                      memcpy(zmqMessage.data(), serialized_data.data(), serialized_data.size());
//                      statuspub->send(zmqMessage, zmq::send_flags::none);
//          }
        
//           static void cmdThread (zmq::context_t *ctx_ )
//           {
               
//                 while (true)
//                 {
                    
//                     zmq::message_t request;
//                     zmq::recv_result_t result = cmdReceive->recv(request);
//                     if (result.has_value()) {
//                           std::string cmd=request.to_string();
//                           std::cout<<cmd<<std::endl;
//                           ct_->cmdQueue->writeCmd(cmd);
//                           cmdReceive->send(zmq::str_buffer("susscess"));
//                     } 
//                     else {
//                        std::cout<<"zmq receive cmd error"<<std::endl;
//                     }
//                     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                 }     
//           }
//           static void pubstatusThread (zmq::context_t *ctx_ ,Centre *ct_)
//           {
               
//                 while (true)
//                 {    
//                      YourMessage youMessage;
//                      youMessage.set_name("zhangyongjing");
//                      youMessage.set_id(100);
//                      std::string serialized_data;
//                      youMessage.SerializeToString(&serialized_data);
                 
//                      zmq::message_t zmqMessage(serialized_data.size());
//                      memcpy(zmqMessage.data(), serialized_data.data(), serialized_data.size());
//                      statuspub->send(zmqMessage, zmq::send_flags::none);
//                      std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                 }     
//           }
//              static void pubDataThread (zmq::context_t *ctx_ ,Centre *ct_)
//           {
                
//                 while (true)
//                 {
//                     // zmq::message_t request;
//                     // zmq::recv_result_t result = dataPub->recv(request);
//                     // if (result.has_value()) {
//                     //       std::string cmd=request.to_string();
//                     //       std::cout<<cmd<<std::endl;
//                     //       ct_->cmdQueue->writeCmd(cmd);
//                     // }
//                     // else {
//                     //    std::cout<<"zmq receive cmd error"<<std::endl;
//                     // }
//                     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                 }     
//           }

//         void run(void)
//         {
//                 ct->init();
//                 auto cmd = std::async(std::launch::async, cmdThread, ctx,ct);
//                 auto pubstatus = std::async(std::launch::async, pubstatusThread, ctx,ct);
//                 auto pubData = std::async(std::launch::async, pubDataThread, ctx,ct);
//                 terminal = std::thread([this]() 
//                 {
//                     while (true) 
//                     {
//                         //指令字符串
//                         std::string cmd;
//                         std::getline(std::cin, cmd);
//                         ct->cmdQueue->writeCmd(cmd);
//                         std::this_thread::sleep_for(std::chrono::milliseconds(100));
//                     }
//                 });
//         }
//         ~ZrcsServer() {
//                 serverTh.join();        
//                 terminal.join();
//              delete ctx; 
//          }
//     };
// }
// #endif
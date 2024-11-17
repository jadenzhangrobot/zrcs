/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:centre.h
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-04
 */
#ifndef CENTRE_H_
#define CENTRE_H_
#include "basenodeInterface.h"
#include "classfactory.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include "controller/rtos/linux.h"
#include "nodeCommunication.h"
#include "timer.h"
#include <algorithm>
#include <any>
#include <cstdint>
#include <cstdlib>
#include <endian.h>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace zrcsSystem {
class Centre {
private:
  //节点指针
  enum TaskScheduling
  {
    STOP,
    RUN,
    ERROR,
  };
  TaskScheduling  taskScheduling=RUN;
  //解析指令的线程
  std::thread cmd_thread;
  //终端字符串接受线程
  std::thread terminal;
  //状态发布线程
  std::thread pubstatus;
  //指令销毁线程
  std::thread exit_cmd;
  std::thread zmq_thread;

  //将要指令对象指针容器
  std::pmr::monotonic_buffer_resource rtCmdPmr;
  std::pmr::monotonic_buffer_resource rtNodePmr;
  std::pmr::vector<Basenode*> rtCmd;
  std::pmr::vector<Basenode*> rtNode;
  //执行完成后的指令对象的指针容器
  //std::pmr::monotonic_buffer_resource exit_rtpmr;
 // std::pmr::vector<Basenode *> exit_rtnodeptr_vector;

  bool terminal_flag = true;
  bool cmd_thread_flag = true;
  bool pubstatus_flag = true;
  bool exitcmd_flag = true;
  bool zmq_flag=true;
 
  //创建一个ros节点
  // Zmq_cmd zmq_cmd; 
  
  
public:
  //指令队列
 static inline std::queue<std::string> cmd_queue;

  std::mutex CmdQueueMutex;
  std::mutex ZmqQueueMute;
  // controller对象指针
  HWAL::Controller *ec_control;
  Basenode *Bnode = nullptr;
 
  Centre():rtCmd(&rtCmdPmr), rtNode(&rtNodePmr),
          ec_control(new HWAL::Controller())      
  {
  }
  Centre(const Centre &) = delete;
  Centre &operator=(const Centre &) = delete;
  ~Centre(void) {
    delete  ec_control;
    terminal_flag = false;
    cmd_thread_flag = false;
    pubstatus_flag = false;
    exitcmd_flag = false;
    zmq_flag=false;
    
    cmd_thread.join();
    terminal.join();
    pubstatus.join();
    exit_cmd.join();
    zmq_thread.join();
  }
  template<class T>
  std::unique_ptr<NodeCommunicaion<T>> createPipeline()
  {
        std::unique_ptr<NodeCommunicaion<T>> nodePipilne(new NodeCommunicaion<T>);
        return nodePipilne;
  }
  void registerObject(std::string cmd)
   {
    //对象名
    std::string class_name;
    // 指令参数字符串
    std::string cmd_param;

    if (cmd.npos != cmd.find_first_of(" --")) 
    {
      class_name = cmd.substr(0, cmd.find_first_of(" --"));

      cmd_param = cmd.substr(cmd.find_first_of(" --")); 
    } 
    else
    {
      class_name = cmd;
    }
    if(!classfactory::getInstance().cmdExist(class_name))
    {
      std::cout<<"cmd不存在"<<std::endl;
    }
    else 
     { 
        if(classfactory::getInstance().getClassByName(class_name).type()==typeid(Basenode*))
        {
                  Basenode *bn =std::any_cast<Basenode*>(classfactory::getInstance().getClassByName(class_name));                   
                  //把节点状态切换到init状态                
                  if (bn->GetTaskState()==Basenode::IDLE) 
                  {
                      bn->registered(ec_control); 
                      bn->SetTaskState(Basenode::INIT);
                  }
                  if (!cmd_param.empty()) 
                  {
                    bn->PushCmdArgs(cmd_param);
                  }                  
                  bn->config();
                  rtCmd.push_back(bn);
        }
        if (classfactory::getInstance().getClassByName(class_name).type()==typeid(CreateNode)) 
        { 
               CreateNode cn =std::any_cast<CreateNode>(classfactory::getInstance().getClassByName(class_name)); 
               Basenode* bn= (*cn)();
               if (bn->GetTaskState()==Basenode::IDLE) 
                  {
                      bn->registered(ec_control); 
                      bn->SetTaskState(Basenode::INIT);
                  }
                  if (!cmd_param.empty()) 
                  {
                    bn->PushCmdArgs(cmd_param);
                  }                  
                  bn->config();
                  rtNode.push_back(bn);
        
        }
                          
            
     }
  }
  void init() {
    // 通过zmq获取命令字符串
    zmq_thread=std::thread([this](){              
                          std::string cmd="Setmode";
                          CmdQueueMutex.lock();
                          cmd_queue.push(cmd);
                          CmdQueueMutex.unlock();
                          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                          std::string cmd1="Enable";
                          CmdQueueMutex.lock();
                          cmd_queue.push(cmd1);
                          CmdQueueMutex.unlock();
                          // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                          // std::string cmd2="JogabsJ --motor=0 --position=0";
                      
                          // CmdQueueMutex.lock();
                          // cmd_queue.push(cmd2);
                          // CmdQueueMutex.unlock();
                          // std::this_thread::sleep_for(std::chrono::milliseconds(4000));
               while (zmq_flag) 
               {
                  //frame_test frame;          
              //     char buffer[128]={};
              //  // std::string buffer;
              //  std::cout<<"----------------------------------"<<std::endl;
              //     int ret= zmq_cmd.sub(buffer); 
              //        printf("%s\n",buffer);        
              //     if(ret<=0)
              //     {
              //        std::cout<<strerror(errno)<<std::endl;
              //     }
              //     else
              //     {
              //           std::string cmd(buffer);
              //           if (cmd!="MotionCtr") 
              //           {
              //                std::cout<<"receive data---  "<<cmd<<std::endl;
              //                //LOGGER_INFO("receive data");
              //                ZmqQueueMute.lock();
              //                cmd_queue.push(cmd);
              //                ZmqQueueMute.unlock();
              //           }
                        
              //     }
                          static int time=200;
                        //   if (cmd_queue.size()>200) 
                        //   {
                        //      time=100000000;
                        //   }
                        //   else 
                        //   {
                        //       time=300;
                        //   }
                        // //   //std::cout<<"**********   "<<cmd_queue.size()<<std::endl;
                        //   std::this_thread::sleep_for(std::chrono::milliseconds(time));
                        //   std::string cmd3="Motion --motor=0 --Tposition=20 --Cposition=0";
                        //   CmdQueueMutex.lock();
                        //   cmd_queue.push(cmd3);
                        //   CmdQueueMutex.unlock();
                        //  std::this_thread::sleep_for(std::chrono::milliseconds(time));

                        //   std::string cmd4="Motion --motor=0 --Tposition=0 --Cposition=20";
                      
                        //   CmdQueueMutex.lock();
                        //   cmd_queue.push(cmd4);
                        //   CmdQueueMutex.unlock();                                
               }

            });

    //终端获取指令字符串
    terminal = std::thread([this]() {
      while (terminal_flag) {
        //指令字符串
        std::string cmd;
        std::getline(std::cin, cmd);
        CmdQueueMutex.lock();
        cmd_queue.push(cmd);
        CmdQueueMutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
    

    //从指令队列里面获取指令字符串
    cmd_thread = std::thread([this]() {
      while (cmd_thread_flag)
      {
        if (!cmd_queue.empty()) 
        {
          std::string cmd_param = cmd_queue.front();
          if (cmd_param == "Stop")
          {         
                taskScheduling=STOP;
          } else if (cmd_param == "Start") 
          {          
             taskScheduling=RUN;
          } else if (cmd_param == "Recover")
          {
                taskScheduling=RUN;
          } else
          {
            this->registerObject(cmd_param);
          }
          cmd_queue.pop();
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    });

    //创建一个实时任务
    ec_control->rtos_->rtos_task_create();
    //把实时节点里面的实时函数放到实时线程中运行
    ec_control->rtos_->real_task([&]() {   
      ec_control->receiveData();
    
     
      switch (taskScheduling) 
      {
        case RUN:
        if (!rtNode.empty())
        {
           for (int i=0; i<rtNode.size(); i++)
           {
                  rtNode[i]->excuteRt();
                  if (rtNode[i]->GetTaskState()==Basenode::FAILURE) 
                  {
                      taskScheduling=ERROR;
                  }
           }
        }
        if (!rtCmd.empty()) 
        {
          Bnode = rtCmd.front();
          if (Bnode != nullptr) 
          {
                if ((Bnode->GetTaskState() == Basenode::INIT)||Bnode->GetTaskState() == Basenode::EXIT)
                {
                      Bnode->init();
                }
                else if(Bnode->GetTaskState() == Basenode::RUNNING) 
                  {   
                      Bnode->excuteRt();
                        
                  } 
                else if (Bnode->GetTaskState() == Basenode::SUCCESS) 
                  {
                    
                    Bnode->exit();
                    rtCmd.erase(rtCmd.begin()); 
                          
                  } 
                else if (Bnode->GetTaskState() == Basenode::FAILURE) 
                  {
                    rtCmd.erase(rtCmd.begin());
                    taskScheduling=ERROR;
                  }
                else 
                  {
                      exit(1);
                  }
           }
          else {             
               rtCmd.erase(rtCmd.begin());
           }
        }
       break;
       case STOP:
       break;
       case ERROR:
       break;
       default:
       break;
          
      }
        ec_control->SendData(); 
    });
  }
};
} 
#endif
/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:centre.h
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-04
 */
#ifndef CENTRE_H_
#define CENTRE_H_
#include <algorithm>
#include <any>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <endian.h>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include "basenodeInterface.h"
#include "classfactory.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include "controller/rtos/linux.h"
#include "nodeCommunication.h"
#include "nodeCommunication.h"
#include "dataType.h"
#include "rt/rt_process.hpp"
namespace zrcsSystem {
class CmdQueue
{
     std::queue<std::string> cmdQueue;
     std::mutex cmdMutex;
     public:
     CmdQueue()
     {}
      void writeCmd(std::string cmd)
      {   
           std::lock_guard<std::mutex> lock(cmdMutex);
           cmdQueue.push(cmd);
      }
      int cmdRead(std::string& cmd)
      { 
            if (!cmdQueue.empty())
            {
                std::lock_guard<std::mutex> lock(cmdMutex);
                cmd = cmdQueue.front();
                cmdQueue.pop();
                return 0;
            }
            return -1;
      }

};
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
  std::thread cmdThread;

  //指令销毁线程
  std::thread exit_cmd;

  //将要指令对象指针容器
  std::pmr::monotonic_buffer_resource rtCmdPmr;
  std::pmr::monotonic_buffer_resource rtNodePmr;
  std::pmr::vector<Basenode*> rtCmd;
  std::pmr::vector<Basenode*> rtNode;
  ZrcsHardware::Controller *control;
  Basenode *Bnode = nullptr;
  bool rtFlag=true;
 // NodeCommunicaion<Motor> motorFeedback; 
public:
  //指令队列
  CmdQueue* cmdQueue;
  
   
  Centre():rtCmd(&rtCmdPmr), rtNode(&rtNodePmr), control(new ZrcsHardware::Controller()),cmdQueue(new CmdQueue())     
  {
     
  }
  Centre(const Centre &) = delete;
  Centre &operator=(const Centre &) = delete;
  ~Centre(void) {
    delete  control;
    delete cmdQueue;
    cmdThread.join();
  }
    // template<class T>
    // std::unique_ptr<NodeCommunicaion<T>> createPipeline()
    // {
    //       std::unique_ptr<NodeCommunicaion<T>> nodePipilne(new NodeCommunicaion<T>);
    //       return nodePipilne;
    // }
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
                      bn->registered(control); 
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
                      bn->registered(control); 
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
    //从指令队列里面获取指令字符串
    cmdThread = std::thread([this]() 
    {
      while (true)
      {
      //   std::uint16_t aaa;
      //   std::memcpy(&aaa,control->inputData.data()+0,2);
         std::string cmd;
         if (cmdQueue->cmdRead(cmd)==0) 
         { 
            if (cmd == "Stop")
            {         
              taskScheduling=STOP;
            } else if (cmd == "Start") 
            {          
              taskScheduling=RUN;
            } else if (cmd == "Recover")
            {
                  taskScheduling=RUN;
            } else
            {
              this->registerObject(cmd);
            }         
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });

    //创建一个实时任务
    control->rtos_->rtos_task_create();
    //把实时节点里面的实时函数放到实时线程中运行
    control->rtos_->real_task([this]() {   
    //control->receiveData();
      
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
     //  control->SendData(); 
    });
  }
};
} 
#endif
/**
 * @copyright 版权所有(c)2024 Glroad公司
 * @filename: centre.h
 * @brief: 系统中心控制类，负责任务调度和节点管理
 * @author: zhangyongjing@oetsky.com
 * @createdate: 2024-01-04
 */
#ifndef CENTRE_H_
#define CENTRE_H_
#include <any>
#include <array>
#include <mutex>
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
#include "common/Shared memory/rt_process.h"
#include "command/Cmdhead.h"



namespace zrcsSystem {
/**
 * @brief 命令队列类，用于线程安全的命令传递
 */
class CmdQueue{
   
     std::mutex cmdMutex;  // 命令队列互斥锁
     std::queue<std::string> cmdQueue;  // 命令队列
     public:
     /**
      * @brief 构造函数
      */
     CmdQueue()
     {

     }
     /**
      * @brief 写入命令到队列
      * @param cmd 要写入的命令字符串
      */
      void writeCmd(std::string& cmd)
      {   
           std::lock_guard<std::mutex> lock(cmdMutex);
           cmdQueue.push(cmd);
      }
      /**
       * @brief 从队列读取命令
       * @param cmd 输出参数，读取到的命令
       * @return 0表示成功，-1表示队列为空
       */
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
/**
 * @brief 系统中心控制类，负责整个系统的任务调度和节点管理
 */
class MotionController {
private:
  // 节点指针
  /**
   * @brief 任务调度状态枚举
   */
  enum TaskScheduling
  {
    STOP = 0,           // 停止
    RUN = 1,            // 运行
    SchedulingError = 2 // 调度错误
  };
  TaskScheduling  taskScheduling=RUN;  // 任务调度状态
  // 命令解析线程
  std::thread cmdThread;

  // 命令销毁线程
  std::thread exit_cmd;

  // 命令对象指针容器
  std::pmr::monotonic_buffer_resource rtCmdPmr;  // 实时命令内存资源
  std::pmr::monotonic_buffer_resource rtNodePmr; // 实时节点内存资源
  std::pmr::vector<Basenode*> rtCmd;   // 实时命令容器
  std::pmr::vector<Basenode*> rtNode;  // 实时节点容器
  ZrcsHardware::Controller *control;   // 硬件控制器指针
  std::array<bool, 100> controlRegister={}; // 控制寄存器
  std::array<bool, 100> statusRegister={};  // 状态寄存器
  RTProcess *rtProcess=nullptr;                // 实时进程指针
  Basenode* cmdNode=nullptr;
  bool rtFlag=true;                    // 实时标志
  bool nrtFlag=true;
 // NodeCommunicaion<Motor> motorFeedback; // 电机反馈通信 
public:
  // 命令队列
  CmdQueue* cmdQueue;
  
  /**
   * @brief 构造函数
   * @param rtProcess_ 实时进程指针
   */
  MotionController(RTProcess *rtProcess_):rtCmd(&rtCmdPmr), rtNode(&rtNodePmr), control(new ZrcsHardware::Controller()),cmdQueue(new CmdQueue())
  {
     rtProcess=rtProcess_;
  }
  
  // 禁用拷贝构造函数
  MotionController(const MotionController &) = delete;
  // 禁用赋值操作符
  MotionController &operator=(const MotionController &) = delete;
  
  /**
   * @brief 析构函数，清理资源
   */
  ~MotionController(void) {
   
    delete  control;
    delete cmdQueue;
    cmdThread.join();
  }
  /**
   * @brief 注册对象到系统中
   * @param cmd 命令字符串，包含类名和参数
   */
  bool registerObject(std::string cmd, std::string& cmdName ,std::string& cmdParam)
   {
    if (cmd.npos != cmd.find_first_of(" --")) 
    {
      cmdName = cmd.substr(0, cmd.find_first_of(" --"));

      cmdParam = cmd.substr(cmd.find_first_of(" --")); 
    } 
    else
    {
      cmdName = cmd;
    }
    if(!classfactory::getInstance().cmdExist(cmdName))
    {
       return false;
    }
     return true;
  }
  /**
   * @brief 运行系统主循环
   * 创建实时任务并启动任务调度器
   */
  void run()
  {
    cmdParsing();
    control->rtos_->rtos_task_create();
    // 将实时节点的实时函数放入实时线程
    control->rtos_->real_task([this]() {   
    // control->receiveData();
     controlRegister= rtProcess->shared_block_->registers.sysControl.load();
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
                      taskScheduling = TaskScheduling::SchedulingError;
                  }
           }
        }
        if(!rtCmd.empty())
        {
          
          if (cmdNode != nullptr) 
          {
                switch (cmdNode->GetTaskState())
                {
                  case Basenode::RTINIT:
                    cmdNode->rtInit();
                    break;                   
                  case Basenode::EXCUTERT:
                    cmdNode->excuteRt();
                    break;                   
                  case Basenode::RTEXIT:
                    cmdNode->rtExit();
                    cmdNode->SetTaskState(Basenode::NRTEXIT);
                    break;
                  case Basenode::FAILURE:                    
                    break;
                  default:                    
                    break;
                }
           }
        }
       break;
       case STOP:
       break;
       case SchedulingError:
       break;
       default:
       break;         
      }  
     rtProcess->shared_block_->registers.sysStatus.store(statusRegister);  
     // control->SendData(); 
    });
  }
  
  /**
   * @brief 命令解析函数
   * 启动命令解析线程，持续从命令队列中读取并处理命令
   */
  void cmdParsing()
  {
      cmdThread = std::thread([this]() {
      while (rtFlag) 
      {
              Command cmd_;
              if(rtProcess->shared_block_->command_queue.pop(cmd_))
              {
                  std::string cmd(cmd_.cmd); 
                  std::string cmdParam={};
                  std::string cmdName={};
                  if(registerObject(cmd,cmdName,cmdParam)==true)
                  {
                        if(classfactory::getInstance().getClassByName(cmdName).type()==typeid(Basenode*))
                        {    
                              cmdNode =std::any_cast<Basenode*>(classfactory::getInstance().getClassByName(cmdName));
                              if (cmdNode!=nullptr) 
                              {
                                 bool running=true;
                                 while (running) 
                                 {
                                    switch (cmdNode->GetTaskState())
                                    {
                                      case Basenode::START:
                                          if (cmdParam.empty()) 
                                          {
                                            cmdNode->PushCmdArgs(cmdParam);                                           
                                          }             
                                      case Basenode::NRTINIT:
                                           cmdNode->nrtInit();
                                           cmdNode->SetTaskState(Basenode::RTINIT);                                      
                                           break;
                                      case Basenode::RTINIT:
                                      case Basenode::EXCUTERT:
                                      case Basenode::RTEXIT:
                                           std::this_thread::sleep_for(std::chrono::milliseconds(10));
                                           break;
                                      case Basenode::NRTEXIT:
                                          running=false;
                                          cmdNode=nullptr;
                                         break;                                          
                                      default:
                                           std::cout<<"指令状态错误"<<std::endl;
                                           break;
                                    }          
                                 }
                                                                                 
                              }                
                        }
                        if (classfactory::getInstance().getClassByName(cmdName).type()==typeid(CreateNode*)) 
                        { 
                              CreateNode* cn =std::any_cast<CreateNode*>(classfactory::getInstance().getClassByName(cmdName));
                              if (cn!=nullptr) 
                              {
                                  
                              }         
                        }          
                  }
                  else
                  {
                       std::cout<<"指令不存在"<<std::endl;
                  }              
          }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    });
  }
};
} 
#endif
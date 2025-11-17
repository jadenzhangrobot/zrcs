/**
 * @copyright 版权所有(c)2024
 * @filename: centre.h
 * @brief: 系统中心控制类，负责任务调度和节点管理
 * @author: 649894200@qq.com
 * @createdate: 2024-01-04
 */
#ifndef NODEMANAGER_H_
#define NODEMANAGER_H_
#include "basenodeInterface.h"
#include "cmdline.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include "controller/rtos/linux.h"
#include "nodeCommunication.h"
#include "nodeFactory.h"
#include <array>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
namespace zrcsSystem {

/**
 * @brief 系统中心控制类，负责整个系统的任务调度和节点管理
 */
class NodeManger{
private:
  // 节点指针
  /**
   * @brief 任务调度状态枚举
   */
  enum TaskScheduling {
    INIT,
    STOP,           // 停止
    RUN,            // 运行
    ERROR  // 调度错误
  };
  TaskScheduling taskScheduling =INIT; // 任务调度状态
  // 命令对象指针容
  ZrcsHardware::Controller *control;             // 硬件控制器指针
  RTProcess *rtProcess = nullptr;                // 实时进程指针
  CmdNode*   cmdNode = nullptr;      
public:

  NodeManger() : rtProcess(new RTProcess("rtMotion")), control(new ZrcsHardware::Controller())
  {
  }

  // 禁用拷贝构造函数
  NodeManger(const NodeManger &) = delete;
  // 禁用赋值操作符
  NodeManger &operator=(const NodeManger &) = delete;

  /**
   * @brief 析构函数，清理资源
   */
  ~NodeManger(void) 
  {
    delete control;
    delete rtProcess;
  }
  /**
   * @brief 运行系统主循环
   * 创建实时任务并启动任务调度器
   */
  void run() 
  {
    rtProcess->initialize();
    for (auto &node : NodeFactory::getInstance().inPutNodes)
      {
        node->registered(control,rtProcess);
      }
      for (auto &node : NodeFactory::getInstance().outPutNodes)
      {
        node->registered(control,rtProcess);
      }

    control->rtos_->rtos_task_create();
    // 将实时节点的实时函数放入实时线程
    control->rtos_->real_task([this]() 
    {
        control->receiveData();
        for (auto &node : NodeFactory::getInstance().inPutNodes)
        {
          node->execute();
        } 
      Command cmd ; 
      switch (taskScheduling) 
      {       
            case INIT:              
                if (rtCmdQueue.pop(cmd))
                {
                   std::string_view cmdName(cmd.cmd);
                   cmdNode =NodeFactory::getInstance().getNodePtr(cmdName).get();
                   cmdNode->registered(control,rtProcess,&cmd);
                   taskScheduling= RUN;
                }               
              break; 
            case RUN:                         
                  if (cmdNode != nullptr)
                  { 
                    if (cmdNode->getCmdStatus() == CmdStatus::COMPLETED) 
                    {
                      cmdNode->setCmdStatus(CmdStatus::INIT);
                      taskScheduling= INIT;
                      // 节点已完成或失败，清理资源
                      cmdNode = nullptr;
                    } 
                    else if(cmdNode->getCmdStatus() == CmdStatus::FAILED)
                    {
                      // 节点仍在运行，继续执行
                      cmdNode = nullptr;
                      taskScheduling= INIT;
                    }
                    else 
                    {
                      // 节点仍在运行，继续执行
                      cmdNode->execute();
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
      for (auto &node : NodeFactory::getInstance().outPutNodes)
      {
        if (node->getNodeStatus() == NodeStatus::RTINIT) 
        {
          node->init();
          node->setNodeStatus(NodeStatus::EXECUTING);
        }
        else if (node->getNodeStatus() == NodeStatus::EXECUTING)
        {
            node->execute();
        }
        else
        {
           INFO_PRINT("%s 执行失败\n",node->getNodeNAME().c_str());  
        }
        
      }
     control->SendData();
    });
  }
};
}
#endif
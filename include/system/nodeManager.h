/**
 * @copyright 版权所有(c)2024
 * @filename: nodeManager.h
 * @brief: 系统中心控制类，负责任务调度和节点管理
 * @author: 649894200@qq.com
 * @createdate: 2024-01-04
 */
#ifndef NODEMANAGER_H_
#define NODEMANAGER_H_
#include "basenodeInterface.h"
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
class NodeManager {
private:
  // 成员变量
  ZrcsHardware::Controller *controller_;         // 硬件控制器指针
  CmdNode* cmdNode_;                             // 当前命令节点指针
  Command cmd_;                                  // 命令对象
  
public:
  RTProcess *rtProcess_;                         // 实时进程指针
  
  NodeManager() : rtProcess_(new RTProcess("rtMotion")), 
                  controller_(new ZrcsHardware::Controller()), 
                  cmdNode_(nullptr)
  {
  }

  // 禁用拷贝构造函数
  NodeManager(const NodeManager &) = delete;
  // 禁用赋值操作符
  NodeManager &operator=(const NodeManager &) = delete;

  /**
   * @brief 析构函数，清理资源
   */
  ~NodeManager(void) 
  {
    delete controller_;
    delete rtProcess_;
  }
  
  void initData()
  {
      AxisCount.store(controller_->axiss.size(), std::memory_order_release); 
  }
  
  /**
   * @brief 运行系统主循环
   * 创建实时任务并启动任务调度器
   */
  void run() 
  {
      rtProcess_->initialize();
      for (auto &node : NodeFactory::getInstance().inPutNodes)
      {
        node->registered(controller_, rtProcess_);
      }
      for (auto &node : NodeFactory::getInstance().outPutNodes)
      {
        node->registered(controller_, rtProcess_);
      }

      initData();

      controller_->rtos_->rtos_task_create();
      // 将实时节点的实时函数放入实时线程
      controller_->rtos_->real_task([this]() 
      {
        controller_->receiveData();
        for (auto &node : NodeFactory::getInstance().inPutNodes)
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
             INFO_PRINT("%s 执行失败\n", node->getNodeName().c_str());  
          }
        }
        
        switch (taskScheduling.load()) 
        {       
              case TaskScheduling::RUN:                         
                    if (cmdNode_ != nullptr)
                    { 
                      if (cmdNode_->getCmdStatus() == CmdStatus::COMPLETED) 
                      {
                        cmdNode_->setCmdStatus(CmdStatus::INIT);
                        // 节点已完成或失败，清理资源
                        cmdNode_ = nullptr;
                      } 
                      else 
                      {
                        // 节点仍在运行，继续执行
                        cmdNode_->execute();
                      }                  
                    }
                    else
                    {
                      if (rtCmdQueue.pop(cmd_)) 
                      {
                           std::string_view cmdName(cmd_.cmd);
                           cmdNode_ = NodeFactory::getInstance().getNodePtr(cmdName).get();
                           cmdNode_->registered(controller_, rtProcess_, &cmd_);
                      }
                    }                            
                  break;
              case TaskScheduling::STOP:
                break;
              case TaskScheduling::RESET:
                  if (cmdNode_ != nullptr) 
                  {
                      cmdNode_->setCmdStatus(CmdStatus::INIT);
                      cmdNode_ = nullptr;
                  }
                  taskScheduling.store(TaskScheduling::RUN, std::memory_order_release);
                break;
              case TaskScheduling::START:            
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
             INFO_PRINT("%s 执行失败\n", node->getNodeName().c_str());  
          }
        }
        
        controller_->sendData();
      });
  }
};
}
#endif

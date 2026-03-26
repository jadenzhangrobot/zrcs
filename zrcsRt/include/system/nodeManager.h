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
#include "nodeCommunication.h"
#include "nodeFactory.h"
#include "model/modelConfig.h"
#include "model/modelFactory.h"
#include "controller/HardwareFactory.h"
#include <array>
#include <memory>
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
  std::unique_ptr<RTProcess> rtProcess_;               // 实时进程指针
  std::unique_ptr<ZrcsHardware::Controller> controller_; // 硬件控制器指针
  std::unique_ptr<ModelConfig> modelConfig_;             // 模型配置
  ModelRegistry modelRegistry_;                          // 多模型注册表
  CmdNode* cmdNode_;                             // 当前命令节点指针
  Command cmd_;                                  // 命令对象

  ShmAccessor shm() { return ShmAccessor(rtProcess_->sharedBlock()); }

public:
  // 供外部访问 RTProcess（如 node->registered）
  RTProcess* rtProcess() const { return rtProcess_.get(); }

  NodeManager() : rtProcess_(std::make_unique<RTProcess>()),
                  controller_(ZrcsHardware::HardwareFactory::createController()),
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
  ~NodeManager() = default;

  /**
   * @brief 停止实时任务，释放资源
   */
  void stop()
  {
      controller_->rtos_->rtos_task_stop();
  }
  
  void initData()
  {
      shm().axisCount().store(controller_->axiss.size(), std::memory_order_release); 
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
        node->registered(controller_.get(), rtProcess_.get());
      }
      for (auto &node : NodeFactory::getInstance().outPutNodes)
      {
        node->registered(controller_.get(), rtProcess_.get());
      }

      initData();

      // 加载运动学模型配置 (在 Controller 之后)
      try {
          modelConfig_ = std::make_unique<ModelConfig>("model.xml");
          modelRegistry_.loadFromConfig(*modelConfig_);
          NodeFactory::getInstance().modelRegistry = &modelRegistry_;
      } catch (const std::exception& e) {
          INFO_PRINT("模型配置加载失败: %s, 继续运行(无运动学)\n", e.what());
      }

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
        
        switch (shm().taskScheduling().load())
        {
              case TaskScheduling::RUN:
                    if (cmdNode_ != nullptr)
                    {
                      if (cmdNode_->getCmdStatus() == CmdStatus::COMPLETED)
                      {
                        // 写回命令完成状态到共享内存，供 NRT BT引擎查询
                        shm().lastCmdSeq().store(cmd_.seq, std::memory_order_release);
                        shm().lastCmdResult().store(0, std::memory_order_release);  // 0=成功
                        cmdNode_->setCmdStatus(CmdStatus::INIT);
                        // 节点已完成或失败，清理资源
                        cmdNode_ = nullptr;
                      }
                      else
                      {
                        // 节点仍在运行，继续执行
                        cmdNode_->execute();
                        // 检查执行后是否失败，写回失败状态
                        if (cmdNode_->getCmdStatus() == CmdStatus::FAILED)
                        {
                            shm().lastCmdSeq().store(cmd_.seq, std::memory_order_release);
                            shm().lastCmdResult().store(1, std::memory_order_release);  // 1=失败
                        }
                      }
                    }
                    else
                    {
                      if (shm().cmdQueue().pop(cmd_))
                      {
                           std::string_view cmdName(cmd_.cmd);
                           cmdNode_ = NodeFactory::getInstance().getNodePtr(cmdName).get();
                           cmdNode_->registered(controller_.get(), rtProcess_.get(), &cmd_);
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
                  shm().taskScheduling().store(TaskScheduling::RUN, std::memory_order_release);
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

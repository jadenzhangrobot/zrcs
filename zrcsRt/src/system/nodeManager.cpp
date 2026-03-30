#include "system/nodeManager.h"
#include "config/projectConfig.h"
#include "command/Cmdhead.h"

namespace zrcsSystem {

void NodeManager::run()
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
        modelConfig_ = std::make_unique<ModelConfig>(
            zrcs::ProjectConfig::prefixedFilename(projectName_, "model.xml"));
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
                         auto nodePtr = NodeFactory::getInstance().getNodePtr(cmdName);
                         if (nodePtr) {
                             cmdNode_ = nodePtr.get();
                             cmdNode_->registered(controller_.get(), rtProcess_.get(), &cmd_);
                         } else {
                             INFO_PRINT("未注册的命令: %s, 已忽略\n", cmd_.cmd);
                             shm().lastCmdSeq().store(cmd_.seq, std::memory_order_release);
                             shm().lastCmdResult().store(1, std::memory_order_release);  // 1=失败
                         }
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

} // namespace zrcsSystem

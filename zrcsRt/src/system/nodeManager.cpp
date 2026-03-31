#include "system/nodeManager.h"
#include "config/projectConfig.h"
#include "command/Cmdhead.h"

namespace zrcsSystem {

void NodeManager::run()
{
    rtProcess_->initialize();

    // 初始化 RT 日志队列，后续 INFO_PRINT/WARN_PRINT/ERROR_PRINT 将写入共享内存
    zrcs::rtlog::setLogQueue(&(rtProcess_->sharedBlock()->logQueue));

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
        WARN_PRINT("模型配置加载失败: %s, 继续运行(无运动学)\n", e.what());
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
           ERROR_PRINT("%s 执行失败\n", node->getNodeName().c_str());
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
                      INFO_PRINT("命令完成: %s(seq=%u)\n", cmd_.cmd, cmd_.seq);
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
                          WARN_PRINT("命令失败: %s(seq=%u)\n", cmd_.cmd, cmd_.seq);
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
                             INFO_PRINT("调度命令: %s(seq=%u)\n", cmd_.cmd, cmd_.seq);
                             cmdNode_ = nodePtr.get();
                             cmdNode_->registered(controller_.get(), rtProcess_.get(), &cmd_);
                         } else {
                             WARN_PRINT("未注册的命令: %s(seq=%u), 已忽略\n", cmd_.cmd, cmd_.seq);
                             shm().lastCmdSeq().store(cmd_.seq, std::memory_order_release);
                             shm().lastCmdResult().store(1, std::memory_order_release);  // 1=失败
                         }
                    }
                  }
                break;
            case TaskScheduling::ERROR_STATE:
                  // 错误状态：清理失败的命令节点，继续消费队列中的恢复性命令
                  if (cmdNode_ != nullptr)
                  {
                    WARN_PRINT("错误状态: 清理命令节点 %s\n", cmdNode_->getNodeName().c_str());
                    cmdNode_->setCmdStatus(CmdStatus::INIT);
                    cmdNode_ = nullptr;
                  }
                  if (shm().cmdQueue().pop(cmd_))
                  {
                       std::string_view cmdName(cmd_.cmd);
                       auto nodePtr = NodeFactory::getInstance().getNodePtr(cmdName);
                       if (nodePtr) {
                           INFO_PRINT("错误恢复: 调度命令 %s(seq=%u)\n", cmd_.cmd, cmd_.seq);
                           cmdNode_ = nodePtr.get();
                           cmdNode_->registered(controller_.get(), rtProcess_.get(), &cmd_);
                           // 恢复到 RUN 状态以执行该命令
                           shm().taskScheduling().store(TaskScheduling::RUN, std::memory_order_release);
                       } else {
                           INFO_PRINT("未注册的命令: %s, 已忽略\n", cmd_.cmd);
                           shm().lastCmdSeq().store(cmd_.seq, std::memory_order_release);
                           shm().lastCmdResult().store(1, std::memory_order_release);
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
           ERROR_PRINT("%s 执行失败\n", node->getNodeName().c_str());
        }
      }

      controller_->sendData();
    });
}

} // namespace zrcsSystem

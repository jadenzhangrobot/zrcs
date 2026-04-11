#include "system/NodeManager.h"
#include "config/ProjectConfig.h"
#include "command/CmdHead.h"

namespace zrcsSystem {

void NodeManager::run()
{
    rtProcess_->initialize();

    zrcs::rtlog::setLogQueue(&(rtProcess_->sharedBlock()->logQueue));

    for (auto &node : factory_.inPutNodes)
    {
        node->registered(controller_.get(), rtProcess_.get());
    }
    for (auto &node : factory_.outPutNodes)
    {
        node->registered(controller_.get(), rtProcess_.get());
    }

    initData();

    try {
        modelConfig_ = std::make_unique<ModelConfig>(
            zrcs::ProjectConfig::prefixedFilename(projectName_, "model.xml"));
        modelRegistry_.loadFromConfig(*modelConfig_);
        factory_.modelRegistry = &modelRegistry_;
        for (auto &node : factory_.inPutNodes)
            node->modelRegistry_ = &modelRegistry_;
        for (auto &node : factory_.outPutNodes)
            node->modelRegistry_ = &modelRegistry_;
    } catch (const std::exception& e) {
        WARN_PRINT("模型配置加载失败: %s, 继续运行(无运动学)\n", e.what());
    }

    controller_->rtos_->rtos_task_create();
    controller_->rtos_->real_task([this]()
    {
        controller_->receiveData();

        for (auto &node : factory_.inPutNodes)
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
                        shm().lastCmdSeq().store(cmd_.seq, std::memory_order_release);
                        shm().lastCmdResult().store(0, std::memory_order_release);
                        INFO_PRINT("命令完成: %s(seq=%u)\n", cmd_.cmd, cmd_.seq);
                        cmdNode_->setCmdStatus(CmdStatus::INIT);
                        cmdNode_ = nullptr;
                    }
                    else
                    {
                        cmdNode_->execute();
                        if (cmdNode_->getCmdStatus() == CmdStatus::FAILED)
                        {
                            WARN_PRINT("命令失败: %s(seq=%u)\n", cmd_.cmd, cmd_.seq);
                            shm().lastCmdSeq().store(cmd_.seq, std::memory_order_release);
                            shm().lastCmdResult().store(1, std::memory_order_release);
                        }
                    }
                }
                else
                {
                    if (shm().cmdQueue().pop(cmd_))
                    {
                        std::string_view cmdName(cmd_.cmd);
                        auto nodePtr = factory_.getNodePtr(cmdName);
                        if (nodePtr) {
                            INFO_PRINT("调度命令: %s(seq=%u)\n", cmd_.cmd, cmd_.seq);
                            cmdNode_ = nodePtr.get();
                            cmdNode_->registered(controller_.get(), rtProcess_.get(), &cmd_);
                            cmdNode_->modelRegistry_ = &modelRegistry_;
                        } else {
                            WARN_PRINT("未注册的命令: %s(seq=%u), 已忽略\n", cmd_.cmd, cmd_.seq);
                            shm().lastCmdSeq().store(cmd_.seq, std::memory_order_release);
                            shm().lastCmdResult().store(1, std::memory_order_release);
                        }
                    }
                }
                break;

            case TaskScheduling::ERROR_STATE:
                if (cmdNode_ != nullptr)
                {
                    WARN_PRINT("错误状态: 清理命令节点 %s\n", cmdNode_->getNodeName().c_str());
                    cmdNode_->setCmdStatus(CmdStatus::INIT);
                    cmdNode_ = nullptr;
                }
                if (shm().cmdQueue().pop(cmd_))
                {
                    std::string_view cmdName(cmd_.cmd);
                    auto nodePtr = factory_.getNodePtr(cmdName);
                    if (nodePtr) {
                        INFO_PRINT("错误恢复: 调度命令 %s(seq=%u)\n", cmd_.cmd, cmd_.seq);
                        cmdNode_ = nodePtr.get();
                        cmdNode_->registered(controller_.get(), rtProcess_.get(), &cmd_);
                        cmdNode_->modelRegistry_ = &modelRegistry_;
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

        for (auto &node : factory_.outPutNodes)
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

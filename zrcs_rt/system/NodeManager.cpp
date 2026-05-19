/**
 * @file NodeManager.cpp
 * @brief Implementation of the RT control loop and command scheduler.
 */

#include "system/NodeManager.h"

#include "command/CmdHead.h"     // IWYU pragma: keep -- triggers REGISTERCMD static initialisers
#include "config/ProjectConfig.h"

namespace zrcsSystem {

void NodeManager::run()
{
    rtProcess_->initialize();

    auto* block = shm();

    // Construct process-local SPSC wrappers.  Bound to the same lifetime as
    // NodeManager, therefore the pointers handed out will never dangle.
    cmdConsumer_ = std::make_unique<zrcs::ShmSPSCConsumer<zrcs::Command, zrcs::kCmdQueueCap>>( block->cmdQueue);

    logProducer_ = std::make_unique<zrcs::ShmSPSCProducer<zrcs::RtLogEntry, zrcs::kLogQueueCap>>(block->logQueue);

    // Register the log producer so that INFO_PRINT and friends write through it.
    zrcs::rtlog::setLogQueue(logProducer_.get());

    for (auto& node : factory_.inPutNodes) 
    {
        node->registered(controller_.get(), rtProcess_.get());
    }

    for (auto& node : factory_.outPutNodes)
    {
        node->registered(controller_.get(), rtProcess_.get());
    }

    initData();

    // Initialisation complete -- switch to RUN so command dispatch is enabled.
    shm()->taskSched.store(zrcs::TaskScheduling::RUN, std::memory_order_release);

    try {
            modelConfig_ = std::make_unique<ModelConfig>(zrcs::ProjectConfig::prefixedFilename(projectName_, "model.xml"));
            modelRegistry_.loadFromConfig(*modelConfig_);
            factory_.modelRegistry = &modelRegistry_;
            for (auto& node : factory_.inPutNodes)
            {
                node->modelRegistry_ = &modelRegistry_;
            }
            for (auto& node : factory_.outPutNodes) 
            {
                node->modelRegistry_ = &modelRegistry_;
            }
      }
    catch (const std::exception& e)
    {
           WARN_PRINT("模型配置加载失败: %s, 继续运行(无运动学)\n", e.what());
    }

    // Publish axis count to shared memory so NRT can size its data structures.
    shm()->axisCount.store(controller_->axes_.size(),
                           std::memory_order_release);

    // Register the RT callback before starting the thread so that the strategy
    // pointer is always valid when the first iteration fires.
    controller_->rtos_->real_task([this]()
    {
        controller_->receiveData();
        taskScheduling_ = shm()->taskSched.load(std::memory_order_acquire);

        auto abortActiveCommand = [this](const char* reason)
        {
            if (cmdNode_ == nullptr)
            {
                return;
            }
            WARN_PRINT("%s: id=%u(seq=%u)\n",reason, static_cast<unsigned>(cmd_.cmdId),cmd_.seq);
            shm()->lastCmdCompletion.store(zrcs::packCmdCompletion(cmd_.seq, 1), std::memory_order_release);
            cmdNode_->setCmdStatus(CmdStatus::INIT);
            cmdNode_->command_ = nullptr;
            cmdNode_ = nullptr;
        };

        auto publishCommandResult = [this](uint8_t result)
        {
            shm()->lastCmdCompletion.store(
                zrcs::packCmdCompletion(cmd_.seq, result),
                std::memory_order_release);
        };

        auto stopContinuousJog = [this]()
        {
            shm()->jogCtrl.active.store(false, std::memory_order_release);
        };

        // ---- Input nodes ----------------------------------------------------
           for (auto& node : factory_.inPutNodes) 
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
        switch (taskScheduling_)
        {
            case zrcs::TaskScheduling::RUN:
            if (cmdNode_ != nullptr) 
                {
                        cmdNode_->execute();
                        if (cmdNode_->getCmdStatus() == CmdStatus::FAILED)
                        {
                            WARN_PRINT("命令失败: id=%u(seq=%u)\n",static_cast<unsigned>(cmd_.cmdId),cmd_.seq);
                            publishCommandResult(1);
                            cmdNode_->setCmdStatus(CmdStatus::INIT);
                            cmdNode_ = nullptr;
                            taskScheduling_ = zrcs::TaskScheduling::ERROR_STATE;
                        }
                        // COMPLETED may have been reached within execute() (status leap).
                        else if (cmdNode_->getCmdStatus() == CmdStatus::COMPLETED)
                        {
                            publishCommandResult(0);
                            INFO_PRINT("命令完成: id=%u(seq=%u)\n",static_cast<unsigned>(cmd_.cmdId), cmd_.seq);
                            cmdNode_->setCmdStatus(CmdStatus::INIT);
                            cmdNode_ = nullptr;
                            break;
                        }
                }
                
                // No active command: try to pop the next one immediately.
                // Skip if we just transitioned to ERROR_STATE — don't re-pop a failing command.
                if (cmdNode_ == nullptr && taskScheduling_ != zrcs::TaskScheduling::ERROR_STATE)
                {
                    if (cmdConsumer_->pop(cmd_)) 
                    {
                        const CmdId cmdId = static_cast<CmdId>(cmd_.cmdId);
                        auto nodePtr = factory_.getNodePtr(cmdId);
                        if (nodePtr) 
                        {
                            INFO_PRINT("调度命令: %s(seq=%u)\n",zrcs::cmdIdToName(cmd_.cmdId), cmd_.seq);
                            cmdNode_ = nodePtr.get();
                            cmdNode_->registered(controller_.get(),rtProcess_.get(), &cmd_);
                            cmdNode_->modelRegistry_ = &modelRegistry_;
                            // Execute init in the same cycle (may also run the
                            // first trajectory step).
                            cmdNode_->execute();
                            if (cmdNode_->getCmdStatus() == CmdStatus::FAILED)
                            {
                                WARN_PRINT("命令失败: id=%u(seq=%u)\n",static_cast<unsigned>(cmd_.cmdId),cmd_.seq);
                                publishCommandResult(1);
                                cmdNode_->setCmdStatus(CmdStatus::INIT);
                                cmdNode_ = nullptr;
                                taskScheduling_ = zrcs::TaskScheduling::ERROR_STATE;
                            }
                           else if (cmdNode_->getCmdStatus() == CmdStatus::COMPLETED)
                           {
                            publishCommandResult(0);
                            INFO_PRINT("命令完成: id=%u(seq=%u)\n",static_cast<unsigned>(cmd_.cmdId), cmd_.seq);
                            cmdNode_->setCmdStatus(CmdStatus::INIT);
                            cmdNode_ = nullptr;
                            break;
                           }
                        } 
                        else if (cmdId != CmdId::INVALID)
                        {
                            WARN_PRINT("未注册的命令: %s(seq=%u), 已忽略\n",zrcs::cmdIdToName(cmd_.cmdId), cmd_.seq);
                            publishCommandResult(1);
                        }
                    }
                }        
                for (auto& node : factory_.outPutNodes) 
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
                break;
            case zrcs::TaskScheduling::ERROR_STATE:
                stopContinuousJog();
                abortActiveCommand("ERROR_STATE abort active command");
                // 错误状态下不自动恢复，等待上位机切换至 RESET 后再继续。
                break;

            case zrcs::TaskScheduling::STOP:
                stopContinuousJog();
                if (!stopHandled_)
                {
                    abortActiveCommand("STOP abort active command");
                    for (auto& axis : controller_->axes_)
                    {
                        axis->powerOff();
                    }
                    stopHandled_ = true;
                }
                break;

            case zrcs::TaskScheduling::RESET:
                stopContinuousJog();
                abortActiveCommand("RESET abort active command");
                stopHandled_ = false;
                taskScheduling_ = zrcs::TaskScheduling::IDLE;
                break;
            case zrcs::TaskScheduling::IDLE:
            default:
                break;


        }

        // ---- Output nodes ---------------------------------------------------
        
        shm()->taskSched.store(taskScheduling_,std::memory_order_release);
        controller_->sendData();
        // Heartbeat: updated solely for RT liveness monitoring.  NRT does not
        // depend on this field for any decision-making.
        static uint64_t heartbeat = 0;
        heartbeat++;
        zrcs::lfl_write(shm()->heartbeat, heartbeat);
    });

    controller_->rtos_->rtos_task_create();
}

} // namespace zrcsSystem

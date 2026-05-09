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
    cmdConsumer_ = std::make_unique<
        zrcs::ShmSPSCConsumer<zrcs::Command, zrcs::kCmdQueueCap>>(
        block->cmdQueue);
    logProducer_ = std::make_unique<
        zrcs::ShmSPSCProducer<zrcs::RtLogEntry, zrcs::kLogQueueCap>>(
        block->logQueue);

    // Register the log producer so that INFO_PRINT and friends write through it.
    zrcs::rtlog::setLogQueue(logProducer_.get());

    for (auto& node : factory_.inPutNodes) {
        node->registered(controller_.get(), rtProcess_.get());
    }

    for (auto& node : factory_.outPutNodes) {
        node->registered(controller_.get(), rtProcess_.get());
    }

    initData();

    // Initialisation complete -- switch to RUN so command dispatch is enabled.
    shm()->taskSched.store(zrcs::TaskScheduling::RUN,
                           std::memory_order_release);

    try {
        modelConfig_ = std::make_unique<ModelConfig>(
            zrcs::ProjectConfig::prefixedFilename(projectName_, "model.xml"));
        modelRegistry_.loadFromConfig(*modelConfig_);
        factory_.modelRegistry = &modelRegistry_;
        for (auto& node : factory_.inPutNodes) {
            node->modelRegistry_ = &modelRegistry_;
        }
        for (auto& node : factory_.outPutNodes) {
            node->modelRegistry_ = &modelRegistry_;
        }
    } catch (const std::exception& e) {
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

        // ---- Input nodes ----------------------------------------------------
        for (auto& node : factory_.inPutNodes) {
            if (node->getNodeStatus() == NodeStatus::RTINIT) {
                node->init();
                node->setNodeStatus(NodeStatus::EXECUTING);
            } else if (node->getNodeStatus() == NodeStatus::EXECUTING) {
                node->execute();
            } else {
                ERROR_PRINT("%s 执行失败\n", node->getNodeName().c_str());
            }
        }

        // ---- Command scheduling ---------------------------------------------
        switch (shm()->taskSched.load(std::memory_order_acquire))
        {
            case zrcs::TaskScheduling::RUN:
                if (cmdNode_ != nullptr) {
                    if (cmdNode_->getCmdStatus() == CmdStatus::COMPLETED) {
                        shm()->lastCmdSeq.store(cmd_.seq,
                                                std::memory_order_release);
                        shm()->lastCmdResult.store(0,
                                                   std::memory_order_release);
                        INFO_PRINT("命令完成: id=%u(seq=%u)\n",
                                   static_cast<unsigned>(cmd_.cmdId), cmd_.seq);
                        cmdNode_->setCmdStatus(CmdStatus::INIT);
                        cmdNode_ = nullptr;
                        // Fall through to pop the next command.
                    } else {
                        cmdNode_->execute();
                        if (cmdNode_->getCmdStatus() == CmdStatus::FAILED) {
                            WARN_PRINT("命令失败: id=%u(seq=%u)\n",
                                       static_cast<unsigned>(cmd_.cmdId),
                                       cmd_.seq);
                            shm()->lastCmdSeq.store(cmd_.seq,
                                                    std::memory_order_release);
                            shm()->lastCmdResult.store(1,
                                                       std::memory_order_release);
                        }
                        // COMPLETED may have been reached within execute()
                        // (status leap).  Handle it in the same cycle.
                        if (cmdNode_->getCmdStatus() == CmdStatus::COMPLETED) {
                            shm()->lastCmdSeq.store(cmd_.seq,
                                                    std::memory_order_release);
                            shm()->lastCmdResult.store(0,
                                                       std::memory_order_release);
                            INFO_PRINT("命令完成: id=%u(seq=%u)\n",
                                       static_cast<unsigned>(cmd_.cmdId),
                                       cmd_.seq);
                            cmdNode_->setCmdStatus(CmdStatus::INIT);
                            cmdNode_ = nullptr;
                            // Fall through to pop the next command.
                        } else {
                            break;
                        }
                    }
                }
                // No active command: try to pop the next one immediately.
                if (cmdNode_ == nullptr) {
                    if (cmdConsumer_->pop(cmd_)) {
                        const CmdId cmdId = static_cast<CmdId>(cmd_.cmdId);
                        auto nodePtr = factory_.getNodePtr(cmdId);
                        if (nodePtr) {
                            INFO_PRINT("调度命令: %s(seq=%u)\n",
                                       zrcs::cmdIdToName(cmd_.cmdId), cmd_.seq);
                            cmdNode_ = nodePtr.get();
                            cmdNode_->registered(controller_.get(),
                                                  rtProcess_.get(), &cmd_);
                            cmdNode_->modelRegistry_ = &modelRegistry_;
                            // Execute init in the same cycle (may also run the
                            // first trajectory step).
                            cmdNode_->execute();
                            if (cmdNode_->getCmdStatus() == CmdStatus::FAILED) {
                                WARN_PRINT("命令失败: id=%u(seq=%u)\n",
                                           static_cast<unsigned>(cmd_.cmdId),
                                           cmd_.seq);
                                shm()->lastCmdSeq.store(cmd_.seq,
                                                        std::memory_order_release);
                                shm()->lastCmdResult.store(1,
                                                           std::memory_order_release);
                            }
                        } else {
                            WARN_PRINT("未注册的命令: %s(seq=%u), 已忽略\n",
                                       zrcs::cmdIdToName(cmd_.cmdId), cmd_.seq);
                            shm()->lastCmdSeq.store(cmd_.seq,
                                                    std::memory_order_release);
                            shm()->lastCmdResult.store(1,
                                                       std::memory_order_release);
                        }
                    }
                }
                break;

            case zrcs::TaskScheduling::ERROR_STATE:
                if (cmdNode_ != nullptr) {
                    WARN_PRINT("错误状态: 清理命令节点 id=%u\n",
                               static_cast<unsigned>(cmd_.cmdId));
                    cmdNode_->setCmdStatus(CmdStatus::INIT);
                    cmdNode_ = nullptr;
                }
                if (cmdConsumer_->pop(cmd_)) {
                    const CmdId cmdId = static_cast<CmdId>(cmd_.cmdId);
                    auto nodePtr = factory_.getNodePtr(cmdId);
                    if (nodePtr) {
                        INFO_PRINT("错误恢复: 调度命令 %s(seq=%u)\n",
                                   zrcs::cmdIdToName(cmd_.cmdId), cmd_.seq);
                        cmdNode_ = nodePtr.get();
                        cmdNode_->registered(controller_.get(),
                                              rtProcess_.get(), &cmd_);
                        cmdNode_->modelRegistry_ = &modelRegistry_;
                        shm()->taskSched.store(zrcs::TaskScheduling::RUN,
                                               std::memory_order_release);
                    } else {
                        INFO_PRINT("未注册的命令: %s, 已忽略\n",
                                   zrcs::cmdIdToName(cmd_.cmdId));
                        shm()->lastCmdSeq.store(cmd_.seq,
                                                std::memory_order_release);
                        shm()->lastCmdResult.store(1,
                                                   std::memory_order_release);
                    }
                }
                break;

            case zrcs::TaskScheduling::STOP:
                break;

            case zrcs::TaskScheduling::RESET:
                if (cmdNode_ != nullptr) {
                    cmdNode_->setCmdStatus(CmdStatus::INIT);
                    cmdNode_ = nullptr;
                }
                shm()->taskSched.store(zrcs::TaskScheduling::RUN,
                                       std::memory_order_release);
                break;

            case zrcs::TaskScheduling::START:
            default:
                break;
        }

        // ---- Output nodes ---------------------------------------------------
        for (auto& node : factory_.outPutNodes) {
            if (node->getNodeStatus() == NodeStatus::RTINIT) {
                node->init();
                node->setNodeStatus(NodeStatus::EXECUTING);
            } else if (node->getNodeStatus() == NodeStatus::EXECUTING) {
                node->execute();
            } else {
                ERROR_PRINT("%s 执行失败\n", node->getNodeName().c_str());
            }
        }

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

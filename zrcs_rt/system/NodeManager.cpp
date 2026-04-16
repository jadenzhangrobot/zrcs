#include "system/NodeManager.h"
#include "config/ProjectConfig.h"
#include "command/CmdHead.h"  // IWYU pragma: keep — triggers REGISTERCMD static initialisers

namespace zrcsSystem {

void NodeManager::run()
{
    rtProcess_->initialize();

    auto* block = shm();

    // 构造进程本地 SPSC 包装器（成员变量，生命周期与 NodeManager 一致，不会悬空）
    cmdConsumer_ = std::make_unique<zrcs::ShmSPSCConsumer<zrcs::Command,    zrcs::kCmdQueueCap>>(block->cmdQueue);
    logProducer_ = std::make_unique<zrcs::ShmSPSCProducer<zrcs::RtLogEntry, zrcs::kLogQueueCap>>(block->logQueue);

    // 注册日志生产者（RT 循环内 INFO_PRINT 等宏通过此指针写共享内存）
    zrcs::rtlog::setLogQueue(logProducer_.get());

    for (auto& node : factory_.inPutNodes)
        node->registered(controller_.get(), rtProcess_.get());

    for (auto& node : factory_.outPutNodes)
        node->registered(controller_.get(), rtProcess_.get());

    initData();

    // 初始化完成，切换到 RUN 状态，使能命令调度
    shm()->taskSched.store(zrcs::TaskScheduling::RUN, std::memory_order_release);

    try {
        modelConfig_ = std::make_unique<ModelConfig>(
            zrcs::ProjectConfig::prefixedFilename(projectName_, "model.xml"));
        modelRegistry_.loadFromConfig(*modelConfig_);
        factory_.modelRegistry = &modelRegistry_;
        for (auto& node : factory_.inPutNodes)
            node->modelRegistry_ = &modelRegistry_;
        for (auto& node : factory_.outPutNodes)
            node->modelRegistry_ = &modelRegistry_;
    } catch (const std::exception& e) {
        WARN_PRINT("模型配置加载失败: %s, 继续运行(无运动学)\n", e.what());
    }

    // 将轴数写入共享内存，供 NRT 启动时读取并初始化模型
     shm()->axisCount.store(controller_->axiss.size(), std::memory_order_release);

    // 先注册策略，再启动线程，避免线程启动时 strategy_ 尚为 nullptr
    controller_->rtos_->real_task([this]()
    {
        controller_->receiveData();

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

        switch (shm()->taskSched.load(std::memory_order_acquire))
        {
            case zrcs::TaskScheduling::RUN:
                if (cmdNode_ != nullptr) {
                    if (cmdNode_->getCmdStatus() == CmdStatus::COMPLETED) {
                        shm()->lastCmdSeq.store(cmd_.seq, std::memory_order_release);
                        shm()->lastCmdResult.store(0, std::memory_order_release);
                        INFO_PRINT("命令完成: id=%u(seq=%u)\n",
                                   static_cast<unsigned>(cmd_.cmdId), cmd_.seq);
                        cmdNode_->setCmdStatus(CmdStatus::INIT);
                        cmdNode_ = nullptr;
                        // 不 break，直接 fall through 到 pop 新命令
                    } else {
                        cmdNode_->execute();
                        if (cmdNode_->getCmdStatus() == CmdStatus::FAILED) {
                            WARN_PRINT("命令失败: id=%u(seq=%u)\n",
                                       static_cast<unsigned>(cmd_.cmdId), cmd_.seq);
                            shm()->lastCmdSeq.store(cmd_.seq, std::memory_order_release);
                            shm()->lastCmdResult.store(1, std::memory_order_release);
                        }
                        // COMPLETED 可能在 execute() 里就到了（状态连跳）
                        if (cmdNode_->getCmdStatus() == CmdStatus::COMPLETED) {
                            shm()->lastCmdSeq.store(cmd_.seq, std::memory_order_release);
                            shm()->lastCmdResult.store(0, std::memory_order_release);
                            INFO_PRINT("命令完成: id=%u(seq=%u)\n",
                                       static_cast<unsigned>(cmd_.cmdId), cmd_.seq);
                            cmdNode_->setCmdStatus(CmdStatus::INIT);
                            cmdNode_ = nullptr;
                            // 不 break，fall through 到 pop 新命令
                        } else {
                            break;
                        }
                    }
                }
                // cmdNode_ == nullptr: 立刻尝试取下一条命令
                if (cmdNode_ == nullptr) {
                    if (cmdConsumer_->pop(cmd_)) {
                        const CmdId cmdId = static_cast<CmdId>(cmd_.cmdId);
                        auto nodePtr = factory_.getNodePtr(cmdId);
                        if (nodePtr) {
                            INFO_PRINT("调度命令: %s(seq=%u)\n",
                                       zrcs::cmdIdToName(cmd_.cmdId), cmd_.seq);
                            cmdNode_ = nodePtr.get();
                            cmdNode_->registered(controller_.get(), rtProcess_.get(), &cmd_);
                            cmdNode_->modelRegistry_ = &modelRegistry_;
                            // 同周期立刻执行 init（甚至第一拍 run）
                            cmdNode_->execute();
                            if (cmdNode_->getCmdStatus() == CmdStatus::FAILED) {
                                WARN_PRINT("命令失败: id=%u(seq=%u)\n",
                                           static_cast<unsigned>(cmd_.cmdId), cmd_.seq);
                                shm()->lastCmdSeq.store(cmd_.seq, std::memory_order_release);
                                shm()->lastCmdResult.store(1, std::memory_order_release);
                            }
                        } else {
                            WARN_PRINT("未注册的命令: %s(seq=%u), 已忽略\n",
                                       zrcs::cmdIdToName(cmd_.cmdId), cmd_.seq);
                            shm()->lastCmdSeq.store(cmd_.seq, std::memory_order_release);
                            shm()->lastCmdResult.store(1, std::memory_order_release);
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
                        cmdNode_->registered(controller_.get(), rtProcess_.get(), &cmd_);
                        cmdNode_->modelRegistry_ = &modelRegistry_;
                        shm()->taskSched.store(zrcs::TaskScheduling::RUN,
                                               std::memory_order_release);
                    } else {
                        INFO_PRINT("未注册的命令: %s, 已忽略\n",
                                   zrcs::cmdIdToName(cmd_.cmdId));
                        shm()->lastCmdSeq.store(cmd_.seq, std::memory_order_release);
                        shm()->lastCmdResult.store(1, std::memory_order_release);
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
    });

    controller_->rtos_->rtos_task_create();
}

} // namespace zrcsSystem

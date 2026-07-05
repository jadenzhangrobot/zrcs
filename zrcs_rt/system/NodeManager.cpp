/**
 * @file NodeManager.cpp
 * @brief RT 控制循环与命令调度器实现。
 */

#include "system/NodeManager.h"

#include "command/CmdHead.h"     // IWYU pragma: keep -- 触发 REGISTERCMD 静态注册
#include "config/ConfigManager.h"
#include "config/ProjectConfig.h"

namespace zrcsSystem {

void NodeManager::run()
{
    rtProcess_->initialize();

    auto* block = shm();
    controller_->bindSharedBlock(block);

    // 构造进程内 SPSC 包装器。它们和 NodeManager 生命周期一致，
    // 因此传出去的指针不会悬空。
    cmdConsumer_ = std::make_unique<zrcs::ShmSPSCConsumer<zrcs::Command, zrcs::kCmdQueueCap>>( block->cmdQueue);

    logProducer_ = std::make_unique<zrcs::ShmSPSCProducer<zrcs::RtLogEntry, zrcs::kLogQueueCap>>(block->logQueue);

    // 注册日志生产者，使 INFO_PRINT 等宏可以写入共享内存日志队列。
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

    // 初始化完成后切换到 RUN，允许命令调度开始工作。
    shm()->taskSched.store(zrcs::TaskScheduling::RUN, std::memory_order_release);

    try {
            // 使用和 HardwareFactory 相同的项目解析规则加载 model.xml。
            // axis/servo 配置已经在控制器创建时被消费，这里只负责把运动学模型
            // 注入给需要模型能力的节点，例如笛卡尔运动命令。
            const auto configManager = zrcs::config::ConfigManager::load(projectName_);
            modelConfig_ = std::make_unique<ModelConfig>((configManager.projectDir() / "model.xml").string());
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

    // 启动线程前先注册 RT 回调，保证第一次周期执行时策略函数已经有效。
    controller_->rtos_->real_task([this]()
    {
        controller_->receiveData();
        const auto observedTaskScheduling = shm()->taskSched.load(std::memory_order_acquire);
        taskScheduling_ = observedTaskScheduling;

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
                        // execute() 内部可能已经直接跳到 COMPLETED。
                        else if (cmdNode_->getCmdStatus() == CmdStatus::COMPLETED)
                        {
                            publishCommandResult(0);
                            INFO_PRINT("命令完成: id=%u(seq=%u)\n",static_cast<unsigned>(cmd_.cmdId), cmd_.seq);
                            cmdNode_->setCmdStatus(CmdStatus::INIT);
                            cmdNode_ = nullptr;
                            break;
                        }
                }
                
                // 当前没有活动命令时，立即尝试取下一条命令。
                // 如果刚切到 ERROR_STATE，则不要再次取出导致失败的命令。
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
                            // 在同一个周期内执行 init，部分命令也可能同时执行第一步轨迹。
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
            case zrcs::TaskScheduling::SHUTDOWN:
                stopContinuousJog();
                abortActiveCommand("SHUTDOWN abort active command");
                break;
            case zrcs::TaskScheduling::IDLE:
            default:
                break;


        }

        // ---- Output nodes ---------------------------------------------------

        auto expectedTaskScheduling = observedTaskScheduling;
        shm()->taskSched.compare_exchange_strong(
            expectedTaskScheduling,
            taskScheduling_,
            std::memory_order_acq_rel,
            std::memory_order_acquire);
        controller_->sendData();
        // 心跳只用于 RT 存活监控，NRT 不依赖这个字段做业务决策。
        static uint64_t heartbeat = 0;
        heartbeat++;
        zrcs::lfl_write(shm()->heartbeat, heartbeat);
    });

    controller_->rtos_->rtos_task_create();
}

} // namespace zrcsSystem

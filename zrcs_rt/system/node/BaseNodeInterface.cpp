/**
 * @file BaseNodeInterface.cpp
 * @brief Implementation of the CmdNode, OutputNode, and InputNode state
 *        machine dispatchers.
 */

#include "system/node/BaseNodeInterface.h"

#include "system/log/RtLog.h"

namespace zrcsSystem {

void CmdNode::execute()
{
    if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::INIT) {
        init();

        if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::INIT) {
            INFO_PRINT("%s 初始化成功\n", nodeName_);
            cmdStatus_.store(CmdStatus::EXECUTING, std::memory_order_release);
        } else {
            // EXIT or FAILED: handle immediately and return to avoid falling
            // through to the duplicate checks below.
            if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::EXIT) {
                exit();
                INFO_PRINT("%s 执行成功\n", nodeName_);
                cmdStatus_.store(CmdStatus::COMPLETED,
                                 std::memory_order_release);
            } else if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::FAILED) {
                ERROR_PRINT("%s(seq=%u) 执行失败\n", nodeName_,
                            command_ ? command_->seq : 0);
                shm()->taskSched.store(zrcs::TaskScheduling::ERROR_STATE,
                                       std::memory_order_release);
            }
            return;
        }
    }

    // EXECUTING -> run the trajectory step.
    if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::EXECUTING) {
        run();
    }

    // EXIT -> cleanup and transition to COMPLETED (same cycle is fine).
    if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::EXIT) {
        exit();
        INFO_PRINT("%s 执行成功\n", nodeName_);
        cmdStatus_.store(CmdStatus::COMPLETED, std::memory_order_release);
    } else if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::FAILED) {
        ERROR_PRINT("%s(seq=%u) 执行失败\n", nodeName_,
                    command_ ? command_->seq : 0);
        shm()->taskSched.store(zrcs::TaskScheduling::ERROR_STATE,
                               std::memory_order_release);
    } else {
        ERROR_PRINT("%s(seq=%u) 状态异常: %d\n", nodeName_,
                    command_ ? command_->seq : 0,
                    static_cast<int>(cmdStatus_.load(
                        std::memory_order_acquire)));
        shm()->taskSched.store(zrcs::TaskScheduling::ERROR_STATE,
                               std::memory_order_release);
    }
}

void OutputNode::execute()
{
    switch (nodeStatus_.load()) {
        case NodeStatus::RTINIT:
            init();
            INFO_PRINT("%s 初始化成功\n", nodeName_);
            nodeStatus_.store(NodeStatus::EXECUTING, std::memory_order_release);
            break;
        case NodeStatus::EXECUTING:
            run();
            break;
        case NodeStatus::FAILED:
            ERROR_PRINT("%s 执行失败\n", nodeName_);
            break;
        default:
            break;
    }
}

void InputNode::execute()
{
    switch (nodeStatus_.load()) {
        case NodeStatus::RTINIT:
            init();
            INFO_PRINT("%s 初始化成功\n", nodeName_);
            nodeStatus_.store(NodeStatus::EXECUTING, std::memory_order_release);
            break;
        case NodeStatus::EXECUTING:
            run();
            break;
        case NodeStatus::FAILED:
            ERROR_PRINT("%s 执行失败\n", nodeName_);
            break;
        default:
            break;
    }
}

} // namespace zrcsSystem

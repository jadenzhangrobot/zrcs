#include "system/base/BaseNodeInterface.h"

namespace zrcsSystem {

void CmdNode::execute()
{
    switch (cmdStatus_.load())
    {
        case CmdStatus::INIT:
            init();
            if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::INIT)
            {
                INFO_PRINT("%s 初始化成功\n", nodeName_);
                cmdStatus_.store(CmdStatus::EXECUTING, std::memory_order_release);
            }
            break;
        case CmdStatus::EXECUTING:
            run();
            break;
        case CmdStatus::EXIT:
            exit();
            INFO_PRINT("%s 执行成功\n", nodeName_);
            cmdStatus_.store(CmdStatus::COMPLETED, std::memory_order_release);
            break;
        case CmdStatus::FAILED:
            ERROR_PRINT("%s(seq=%u) 执行失败\n", nodeName_, command_ ? command_->seq : 0);
            shm().taskScheduling().store(TaskScheduling::ERROR_STATE, std::memory_order_release);
            break;
        default:
            break;
    }
}

void OutputNode::execute()
{
    switch (nodeStatus_.load())
    {
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
    switch (nodeStatus_.load())
    {
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

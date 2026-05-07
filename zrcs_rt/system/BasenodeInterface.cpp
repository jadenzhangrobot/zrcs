#include "system/base/BaseNodeInterface.h"

namespace zrcsSystem {

void CmdNode::execute()
{
    

  
    if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::INIT)
    {
        init();
        
        if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::INIT)
        {
            INFO_PRINT("%s 初始化成功\n", nodeName_);
            cmdStatus_.store(CmdStatus::EXECUTING, std::memory_order_release);
        }
        else
        {
            // EXIT 或 FAILED：处理完即返回，避免落到下面重复处理
            if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::EXIT)
            {
                exit();
                INFO_PRINT("%s 执行成功\n", nodeName_);
                cmdStatus_.store(CmdStatus::COMPLETED, std::memory_order_release);
            }
            else if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::FAILED)
            {
                ERROR_PRINT("%s(seq=%u) 执行失败\n", nodeName_, command_ ? command_->seq : 0);
                shm()->taskSched.store(zrcs::TaskScheduling::ERROR_STATE, std::memory_order_release);
            }
            return;
        }
    }

    // EXECUTING → 执行轨迹
    if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::EXECUTING)
    {
        run();
    }

    // EXIT → 清理，推进到 COMPLETED（同一拍完成，不等下一个周期）
    if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::EXIT)
    {
        exit();
        INFO_PRINT("%s 执行成功\n", nodeName_);
        cmdStatus_.store(CmdStatus::COMPLETED, std::memory_order_release);
    }
    else if (cmdStatus_.load(std::memory_order_acquire) == CmdStatus::FAILED)
    {
        ERROR_PRINT("%s(seq=%u) 执行失败\n", nodeName_, command_ ? command_->seq : 0);
        shm()->taskSched.store(zrcs::TaskScheduling::ERROR_STATE, std::memory_order_release);
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

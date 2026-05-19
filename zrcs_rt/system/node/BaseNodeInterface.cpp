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
     // 1. 周期开始时，只做一次原子读取，赋值给局部变量

        if (cmdStatus_ == CmdStatus::INIT)
        {
            if(init())
            {
                INFO_PRINT("%s 初始化指令成功\n", nodeName_);
                // 直接修改局部变量，这样下一个 if 就能立刻接住
                cmdStatus_ = CmdStatus::RUNNING; 
            }
            else
            {     
                ERROR_PRINT("%s(seq=%u) 初始化指令失败\n", nodeName_, command_ ? command_->seq : 0);
                cmdStatus_ = CmdStatus::FAILED;
            }
        }

        // 因为上面 currentStatus 可能被改成了 RUNNING，所以这里立刻就会被判定为 true
        if (cmdStatus_ == CmdStatus::RUNNING)
        {
            RunResult result = run();
            if (result == RunResult::EXECUTING)
            {
            }
            else if (result == RunResult::SUCCESS)
            {
                INFO_PRINT("%s 执行指令成功\n", nodeName_);
                cmdStatus_ = CmdStatus::EXIT;
            }
            else
            {
                ERROR_PRINT("%s 执行指令失败\n", nodeName_);
                cmdStatus_ = CmdStatus::FAILED;
            }
        }

        if (cmdStatus_ == CmdStatus::EXIT)
        {
            if (exit())
            {
                INFO_PRINT("%s 退出指令成功\n", nodeName_);
                // 假设有个结束状态
                cmdStatus_ = CmdStatus::COMPLETED; 
            }
            else 
            { 
                ERROR_PRINT("%s 退出指令失败\n", nodeName_);
                cmdStatus_ = CmdStatus::FAILED;  
            }
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

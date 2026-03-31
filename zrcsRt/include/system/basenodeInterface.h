/**
 * @copyright Copyright(c) 2024 Glroad Corporation
 * @filename: basenodeInterface.h
 * @brief: Base node interface definitions for ZRCS system
 * @author: zhangyongjing@oetsky.com
 * @createdate: 2024-01-08
 */
#ifndef BASENODE_INTERFACE_H_
#define BASENODE_INTERFACE_H_
#include "config/cmdArgs.h"
#include "sharedMemory/rtProcess.h"
#include "sharedMemory/sharedData.h"
#include "sharedMemory/registerInfo.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include <atomic>
#include <cstdint>
#include <string>
#include "system/rtLog.h"
#include "config/parameter.h"

namespace zrcsSystem {

class Basenode {
public:
    uint64_t nodeCount_;
    char nodeName_[32];
    std::string cmdParam_;
    ZrcsHardware::Controller* controller_;
    RTProcess *rtProcess_;
    Command* command_;

    Basenode() : nodeCount_(0), nodeName_{}, cmdParam_(), 
                 controller_(nullptr), rtProcess_(nullptr), command_(nullptr)
    {
    }
    virtual ~Basenode() = default;
    
    void registered(ZrcsHardware::Controller* ct, RTProcess *rtProcess, Command* command) 
    {
        controller_ = ct;
        rtProcess_ = rtProcess;
        command_ = command;
    }
    
    void registered(ZrcsHardware::Controller* ct, RTProcess *rtProcess)
    {
        controller_ = ct;
        rtProcess_ = rtProcess;
    }

    ShmAccessor shm() { return ShmAccessor(rtProcess_->sharedBlock()); }
    
    // 获取节点名字
    std::string getNodeName(void) const
    {
        return nodeName_;
    }
    
    std::uint64_t getNodeCount() const
    {
        return nodeCount_;
    }
};

enum class CmdStatus {
    START,         // 节点已创建
    INIT,          // 非实时初始化中
    EXECUTING,     // 核心逻辑执行中
    EXIT,          // 非实时退出中
    COMPLETED,     // 执行成功完成
    FAILED         // 执行失败
};


class CmdNode : public Basenode {
public:
    std::atomic<CmdStatus> cmdStatus_;
    
    CmdNode() : cmdStatus_(CmdStatus::INIT) {}
    virtual ~CmdNode() = default;
    
    // 非实时初始化
    virtual void init() = 0;
    virtual void run() = 0;
    // 非实时退出
    virtual void exit() = 0;
    
    void execute(); 
    
    // 获取命令节点状态
    CmdStatus getCmdStatus() const noexcept {
        return cmdStatus_.load(std::memory_order_acquire);
    }
    
    // 设置命令节点状态
    void setCmdStatus(CmdStatus status) {
        cmdStatus_.store(status, std::memory_order_release);
    }
};


enum class NodeStatus 
{
    RTINIT,        // 实时初始化中
    EXECUTING,     // 持续运行中
    FAILED         // 运行失败
};

// 持久性节点：持续运行的节点
class OutputNode : public Basenode 
{
public:
    std::atomic<NodeStatus> nodeStatus_;
    
    OutputNode() : nodeStatus_(NodeStatus::RTINIT) {}
    virtual ~OutputNode() = default;
    virtual void init() = 0;
    virtual void run() = 0;
   
    void execute();
    
    NodeStatus getNodeStatus() const
    {
        return nodeStatus_.load(std::memory_order_acquire);
    }
    
    void setNodeStatus(NodeStatus status) 
    {
        nodeStatus_.store(status, std::memory_order_release);
    }
};


class InputNode : public Basenode {
public:
    std::atomic<NodeStatus> nodeStatus_;

    InputNode() : nodeStatus_(NodeStatus::RTINIT) {}
    virtual ~InputNode() = default;
    virtual void init() = 0;
    virtual void run() = 0;

    virtual void execute();

    NodeStatus getNodeStatus() const
    {
        return nodeStatus_.load(std::memory_order_acquire);
    }

    void setNodeStatus(NodeStatus status)
    {
        nodeStatus_.store(status, std::memory_order_release);
    }
};


} // namespace zrcsSystem
#endif

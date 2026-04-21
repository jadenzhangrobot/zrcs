/**
 * @copyright Copyright(c) 2024 Glroad Corporation
 * @filename: BaseNodeInterface.h
 * @brief: Base node interface definitions for ZRCS system
 * @author: zhangyongjing@oetsky.com
 * @createdate: 2024-01-08
 */
#pragma once

#include "shared_memory/RtProcess.h"   // zrcs::RtProcess / RTProcess, SharedBlock
#include "system/CmdIds.h"          // 参数索引枚举、CmdId、TaskScheduling（via ShmLayout）
#include "controller/Controller.h"
#include "system/RtLog.h"
#include "config/Parameter.h"
#include <atomic>
#include <cstdint>
#include <string>

class ModelRegistry;

namespace zrcsSystem {

class Basenode {
public:
    uint64_t nodeCount_;
    char nodeName_[32];
    std::string cmdParam_;
    ZrcsHardware::Controller* controller_;
    RTProcess *rtProcess_;
    zrcs::Command* command_;
    ModelRegistry* modelRegistry_;

    Basenode() : nodeCount_(0), nodeName_{}, cmdParam_(),
                 controller_(nullptr), rtProcess_(nullptr),
                 command_(nullptr), modelRegistry_(nullptr)
    {
    }
    virtual ~Basenode() = default;

    void registered(ZrcsHardware::Controller* ct, RTProcess *rtProcess, zrcs::Command* command)
    {
        controller_    = ct;
        rtProcess_     = rtProcess;
        command_       = command;
    }

    void registered(ZrcsHardware::Controller* ct, RTProcess *rtProcess)
    {
        controller_ = ct;
        rtProcess_  = rtProcess;
    }

    // 直接返回 SharedBlock 指针，调用方通过 shm()->field 访问
    zrcs::SharedBlock* shm() const noexcept { return rtProcess_->sharedBlock(); }

    std::string getNodeName() const { return nodeName_; }
    std::uint64_t getNodeCount() const { return nodeCount_; }
};

enum class CmdStatus {
    START,
    INIT,
    EXECUTING,
    EXIT,
    COMPLETED,
    FAILED
};


class CmdNode : public Basenode {
public:
    std::atomic<CmdStatus> cmdStatus_;

    CmdNode() : cmdStatus_(CmdStatus::INIT) {}
    virtual ~CmdNode() = default;

    virtual void init() = 0;
    virtual void run() = 0;
    virtual void exit() = 0;

    void execute();

    CmdStatus getCmdStatus() const noexcept {
        return cmdStatus_.load(std::memory_order_acquire);
    }

    void setCmdStatus(CmdStatus status) {
        cmdStatus_.store(status, std::memory_order_release);
    }
};


enum class NodeStatus
{
    RTINIT,
    EXECUTING,
    FAILED
};

class OutputNode : public Basenode
{
public:
    std::atomic<NodeStatus> nodeStatus_;

    OutputNode() : nodeStatus_(NodeStatus::RTINIT) {}
    virtual ~OutputNode() = default;
    virtual void init() = 0;
    virtual void run() = 0;

    void execute();

    NodeStatus getNodeStatus() const {
        return nodeStatus_.load(std::memory_order_acquire);
    }

    void setNodeStatus(NodeStatus status) {
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

    NodeStatus getNodeStatus() const {
        return nodeStatus_.load(std::memory_order_acquire);
    }

    void setNodeStatus(NodeStatus status) {
        nodeStatus_.store(status, std::memory_order_release);
    }
};


} // namespace zrcsSystem

/**
 * @copyright Copyright(c) 2024 Glroad Corporation
 * @file BaseNodeInterface.h
 * @brief Base node interface definitions for the ZRCS real-time system.
 *
 * Defines the type hierarchy for all nodes that participate in the RT control
 * loop: Basenode (common base), CmdNode (command-driven, stateful execution),
 * OutputNode and InputNode (periodic data-plane nodes).
 *
 * @author zhangyongjing@oetsky.com
 * @date 2024-01-08
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "config/CmdDefine.h"
#include "config/Parameter.h"
#include "controller/Controller.h"
#include "shared_memory/RtProcess.h"
#include "system/log/RtLog.h"

class ModelRegistry;

namespace zrcsSystem {

/**
 * @brief Common base for all node types (command, input, output).
 *
 * Stores the references injected during registration (controller, RT process,
 * command pointer, model registry) and provides shared utilities such as
 * direct shared-memory access.
 *
 * @note All data members are public for direct access by derived classes and
 *       the NodeManager scheduler.  Treat them as read-only after registration.
 */
class Basenode {
protected:
    /// Hook called after the node has been registered with controller / RT process.
    virtual void onRegistered() {}

public:
    uint64_t nodeCount_;            ///< Monotonic execution counter.
    char     nodeName_[32];         ///< Human-readable node name (set by subclass).
    std::string cmdParam_;          ///< Optional command parameter string.

    ZrcsHardware::Controller* controller_{nullptr};    ///< Axis-level I/O.
    RTProcess*                rtProcess_{nullptr};      ///< RT process (shared memory).
    zrcs::Command*            command_{nullptr};        ///< Current command (CmdNode only).
    ModelRegistry*            modelRegistry_{nullptr};  ///< Kinematic model registry.

    Basenode()
        : nodeCount_(0), nodeName_{}, cmdParam_(),
          controller_(nullptr), rtProcess_(nullptr),
          command_(nullptr), modelRegistry_(nullptr)
    {
    }

    virtual ~Basenode() = default;

    /// Register the node for command-driven execution (CmdNode path).
    /// @param ct        Hardware controller.
    /// @param rtProcess RT process handle.
    /// @param command   Command descriptor from shared memory.
    void registered(ZrcsHardware::Controller* ct, RTProcess* rtProcess,
                    zrcs::Command* command)
    {
        controller_ = ct;
        rtProcess_  = rtProcess;
        command_    = command;
        onRegistered();
    }

    /// Register the node for periodic execution (InputNode / OutputNode path).
    /// @param ct        Hardware controller.
    /// @param rtProcess RT process handle.
    void registered(ZrcsHardware::Controller* ct, RTProcess* rtProcess)
    {
        controller_ = ct;
        rtProcess_  = rtProcess;
        onRegistered();
    }

    /// @return Direct pointer to the shared-memory block for fast field access.
    zrcs::SharedBlock* shm() const noexcept { return rtProcess_->sharedBlock(); }

    /// @return Null-terminated node name.
    std::string getNodeName() const { return nodeName_; }

    /// @return Monotonic execution counter value.
    std::uint64_t getNodeCount() const { return nodeCount_; }
};

/**
 * @brief Execution status of a CmdNode.
 *
 * State transitions:
 *   INIT -> EXECUTING -> COMPLETED
 *   Any state can jump to FAILED on error.
 */
enum class CmdStatus {
    START,      ///< Reserved, not yet in use.
    INIT,       ///< Initialising trajectory parameters.
    EXECUTING,  ///< Running the trajectory step by step.
    COMPLETED,  ///< Terminal: command finished successfully.
    FAILED      ///< Terminal: command encountered an unrecoverable error.
};

/**
 * @brief Command-driven node with a built-in state machine.
 *
 * Subclasses implement init() / run() / exit() to drive the command through
 * INIT -> EXECUTING -> COMPLETED.  The base execute() method reads
 * the current status and dispatches to the appropriate virtual.
 *
 * The status is stored in an atomic to allow lock-free reads from the NRT
 * side (e.g., for progress monitoring).
 */
class CmdNode : public Basenode {
public:
    std::atomic<CmdStatus> cmdStatus_;  ///< Current state of the command FSM.

    CmdNode() : cmdStatus_(CmdStatus::INIT) {}
    virtual ~CmdNode() = default;

    /// Called once when the node first becomes active.  Set up trajectory.
    virtual void init() = 0;

    /// Called every control cycle while in EXECUTING state.
    virtual void run() = 0;

    /// Called once when transitioning to COMPLETED.  Tear down resources.
    virtual void exit() = 0;

    /// State-machine dispatcher.  Reads cmdStatus_ and invokes the
    /// appropriate phase (init / run / exit).
    void execute();

    /// @return Current command status (acquire semantics).
    CmdStatus getCmdStatus() const noexcept {
        return cmdStatus_.load(std::memory_order_acquire);
    }

    /// @param status New command status to set (release semantics).
    void setCmdStatus(CmdStatus status) {
        cmdStatus_.store(status, std::memory_order_release);
    }
};

/**
 * @brief Periodic node status.
 */
enum class NodeStatus {
    RTINIT,     ///< Not yet initialised or pending initialisation.
    EXECUTING,  ///< Running normally.
    FAILED      ///< Terminal: unrecoverable error.
};

/**
 * @brief Periodic output node (data publishing, status reporting, etc.).
 *
 * The execute() method runs init() once, then calls run() every subsequent
 * cycle.  If the node enters FAILED state, a diagnostic is logged.
 */
class OutputNode : public Basenode {
public:
    std::atomic<NodeStatus> nodeStatus_;  ///< Current run status.

    OutputNode() : nodeStatus_(NodeStatus::RTINIT) {}
    virtual ~OutputNode() = default;

    /// One-time initialisation.
    virtual void init() = 0;

    /// Per-cycle work function.
    virtual void run() = 0;

    /// Dispatcher: calls init() or run() based on current status.
    void execute();

    /// @return Current node status.
    NodeStatus getNodeStatus() const {
        return nodeStatus_.load(std::memory_order_acquire);
    }

    /// @param status New node status.
    void setNodeStatus(NodeStatus status) {
        nodeStatus_.store(status, std::memory_order_release);
    }
};

/**
 * @brief Periodic input node (data acquisition, sensor polling, etc.).
 *
 * Mirrors OutputNode in structure; separated to allow the scheduler to
 * process inputs and outputs in distinct phases.
 */
class InputNode : public Basenode {
public:
    std::atomic<NodeStatus> nodeStatus_;  ///< Current run status.

    InputNode() : nodeStatus_(NodeStatus::RTINIT) {}
    virtual ~InputNode() = default;

    /// One-time initialisation.
    virtual void init() = 0;

    /// Per-cycle work function.
    virtual void run() = 0;

    /// Dispatcher: calls init() or run() based on current status.
    virtual void execute();

    /// @return Current node status.
    NodeStatus getNodeStatus() const {
        return nodeStatus_.load(std::memory_order_acquire);
    }

    /// @param status New node status.
    void setNodeStatus(NodeStatus status) {
        nodeStatus_.store(status, std::memory_order_release);
    }
};

} // namespace zrcsSystem

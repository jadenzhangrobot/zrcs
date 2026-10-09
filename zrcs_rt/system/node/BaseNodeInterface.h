/**
 * @copyright Copyright(c) 2024 Glroad Corporation
 * @file BaseNodeInterface.h
 * @brief Base node interface definitions for the ZRCS real-time system.
 *
 * Defines the type hierarchy for all nodes that participate in the RT control
 * loop: Basenode (common base), CmdNode (command-driven, stateful execution),
 * PeriodicNode (always-on, phase-ordered data-plane nodes).
 *
 * @author zhangyongjing@oetsky.com
 * @date 2024-01-08
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "config/CmdDefine.h"
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

    /// Register the node for periodic execution (PeriodicNode path).
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
    INIT,       ///< Initialising trajectory parameters.
    RUNNING,  ///< Running the trajectory step by step.
    EXIT,
    COMPLETED,  ///< Terminal: command finished successfully.
    FAILED      ///< Terminal: command encountered an unrecoverable error.
};

enum class RunResult 
{
    SUCCESS,     ///< Not yet initialised or pending initialisation.
    EXECUTING,  ///< Running normally.
    FAILED      ///< Terminal: unrecoverable error.
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
    CmdStatus cmdStatus_;  ///< Current state of the command FSM.
  //  RunResult runResult_;   ///< Current run result.

    CmdNode() : cmdStatus_(CmdStatus::INIT) {}
    virtual ~CmdNode() = default;

    /// Called once before the real-time task starts. Override this hook to
    /// allocate command-owned resources or cache configuration needed by
    /// init() and run().
    virtual bool prepare() { return true; }

    /// Called once when the node first becomes active.  Set up trajectory.
    virtual bool init() = 0;

    /// Called every control cycle while in EXECUTING state.
    virtual RunResult run() = 0;

    /// Called once when transitioning to COMPLETED.  Tear down resources.
    virtual bool exit() = 0;

    /// State-machine dispatcher.  Reads cmdStatus_ and invokes the
    /// appropriate phase (init / run / exit).
    void execute();

    /// @return Current command status (acquire semantics).
    CmdStatus getCmdStatus() const noexcept 
    {
        return cmdStatus_;
    }

    /// @param status New command status to set (release semantics).
    void setCmdStatus(CmdStatus status) 
    {
        cmdStatus_ = status;
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
 * @brief Execution phase of a periodic node within the RT cycle.
 *
 * Hard constraint: all INPUT nodes run before the command / trajectory phase,
 * and all OUTPUT nodes run after it.  Guarantees a deterministic
 * "acquire data -> plan -> publish data" ordering within each cycle.
 */
enum class NodePhase {
    INPUT,      ///< Data acquisition / sensor polling (before commands).
    OUTPUT      ///< Data publishing / IO driving (after commands).
};

/**
 * @brief Periodic (always-on) node: data acquisition, publishing, etc.
 *
 * Replaces the former InputNode and OutputNode.  Each cycle execute()
 * runs init() once, then run() every subsequent cycle.  Execution order
 * within a phase is governed by execOrder_ (ascending); cross-phase order
 * is fixed by phase_ (INPUT before OUTPUT).
 */
class PeriodicNode : public Basenode {
public:
    std::atomic<NodeStatus> nodeStatus_{NodeStatus::RTINIT};  ///< Current run status.
    NodePhase phase_{NodePhase::INPUT};   ///< Hard phase: INPUT runs before OUTPUT.
    uint32_t  execOrder_{0};              ///< Relative order within phase, ascending.

    PeriodicNode() = default;
    virtual ~PeriodicNode() = default;

    /// One-time initialisation.
    virtual void init() = 0;

    /// Per-cycle work function.
    virtual void run() = 0;

    /// Dispatcher: calls init() or run() based on current status.
    void execute();

    /// @return Current node status.
    NodeStatus getNodeStatus() const 
    {
        return nodeStatus_.load(std::memory_order_acquire);
    }

    /// @param status New node status.
    void setNodeStatus(NodeStatus status) 
    {
        nodeStatus_.store(status, std::memory_order_release);
    }
};

} // namespace zrcsSystem

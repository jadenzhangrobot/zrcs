/**
 * @file NodeManager.h
 * @brief Real-time node manager -- owns the RT process, controller, model
 *        configuration, and node factory.  Orchestrates the main control loop
 *        including command scheduling and I/O node execution.
 */
#pragma once

#include <memory>
#include <string>

#include "controller/Controller.h"
#include "controller/HardwareFactory.h"
#include "model/ModelConfig.h"
#include "model/ModelFactory.h"
#include "system/node/BaseNodeInterface.h"
#include "system/node/NodeFactory.h"

namespace zrcsSystem {

/**
 * @brief Top-level manager for the real-time control subsystem.
 *
 * Owns the RT process (shared memory + kernel threads), the hardware
 * controller (drives axes and reads feedback), the model configuration
 * (kinematics) and the node factory (command / I/O node registry).
 *
 * The main control loop (run()) is registered as an RT task and cycles
 * through:
 *   1. Input nodes   -- data acquisition
 *   2. Command node  -- trajectory execution (if one is active or queued)
 *   3. Output nodes  -- data publishing
 *   4. Controller I/O
 *
 * @note Not copyable or movable.
 */
class NodeManager {
public:
    /// Construct the manager, creating the RT process and hardware controller.
    /// @param projectName Project identifier used to locate config files.
    explicit NodeManager(const std::string& projectName = "")
        : projectName_(projectName),
          rtProcess_(std::make_unique<RTProcess>()),
          controller_(ZrcsHardware::HardwareFactory::createController(projectName))
    {
    }

    NodeManager(const NodeManager&) = delete;
    NodeManager& operator=(const NodeManager&) = delete;
    ~NodeManager() = default;

    /// @return Raw pointer to the underlying RTProcess (for external use).
    RTProcess* rtProcess() const { return rtProcess_.get(); }

    /// Stop the RT kernel task gracefully.
    void stop() { controller_->rtos_->rtos_task_stop(); }

    /// Write initial axis count into shared memory so NRT consumers can
    /// size their data structures before the control loop starts.
    void initData()
    {
        shm()->axisCount.store(
            static_cast<uint8_t>(controller_->axes_.size()),
            std::memory_order_release);
    }

    /// Build the RT control loop, register it as an RT task, and start.
    void run();

private:
    /// @brief Convenience accessor for the shared-memory block.
    zrcs::SharedBlock* shm() const noexcept { return rtProcess_->sharedBlock(); }
    zrcs::TaskScheduling taskScheduling_;
    // ---- Owned objects -------------------------------------------------------
    std::string                                 projectName_;
    std::unique_ptr<RTProcess>                  rtProcess_;
    std::unique_ptr<ZrcsHardware::Controller>   controller_;
    std::unique_ptr<ModelConfig>                modelConfig_;
    ModelRegistry                               modelRegistry_;
    NodeFactory                                 factory_;
    CmdNode*                                    cmdNode_{nullptr};
    zrcs::Command                               cmd_;
    bool                                        stopHandled_{false};

    /// SPSC queue wrappers (initialised in run(), same lifetime as NodeManager).
    /// unique_ptr defers construction until the SharedBlock pointer is available.
    std::unique_ptr<zrcs::ShmSPSCConsumer<zrcs::Command,    zrcs::kCmdQueueCap>> cmdConsumer_;
    std::unique_ptr<zrcs::ShmSPSCProducer<zrcs::RtLogEntry, zrcs::kLogQueueCap>> logProducer_;
};

} // namespace zrcsSystem

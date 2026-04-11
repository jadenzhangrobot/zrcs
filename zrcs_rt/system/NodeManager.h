#pragma once

#include "system/base/BaseNodeInterface.h"
#include "controller/Controller.h"
#include "NodeCommunication.h"
#include "NodeFactory.h"
#include "model/ModelConfig.h"
#include "model/ModelFactory.h"
#include "controller/HardwareFactory.h"
#include "config/ProjectConfig.h"
#include <memory>
#include <vector>

namespace zrcsSystem {

class NodeManager {
private:
    std::string projectName_;
    std::unique_ptr<RTProcess>                    rtProcess_;
    std::unique_ptr<ZrcsHardware::Controller>     controller_;
    std::unique_ptr<ModelConfig>                  modelConfig_;
    ModelRegistry                                 modelRegistry_;
    NodeFactory                                   factory_;
    CmdNode*                                      cmdNode_{nullptr};
    zrcs::Command                                 cmd_;

    // 进程本地 SPSC 包装器：在 run() 中初始化，生命周期与 NodeManager 一致
    // 使用 unique_ptr 延迟构造（需要 shm() 指针才能初始化）
    std::unique_ptr<zrcs::ShmSPSCConsumer<zrcs::Command,    zrcs::kCmdQueueCap>> cmdConsumer_;
    std::unique_ptr<zrcs::ShmSPSCProducer<zrcs::RtLogEntry, zrcs::kLogQueueCap>> logProducer_;

    zrcs::SharedBlock* shm() const noexcept { return rtProcess_->sharedBlock(); }

public:
    RTProcess* rtProcess() const { return rtProcess_.get(); }

    explicit NodeManager(const std::string& projectName = "")
        : projectName_(projectName),
          rtProcess_(std::make_unique<RTProcess>()),
          controller_(ZrcsHardware::HardwareFactory::createController(projectName))
    {
    }

    NodeManager(const NodeManager&) = delete;
    NodeManager& operator=(const NodeManager&) = delete;
    ~NodeManager() = default;

    void stop() { controller_->rtos_->rtos_task_stop(); }

    void initData() {
        shm()->axisCount.store(
            static_cast<uint8_t>(controller_->axiss.size()),
            std::memory_order_release);
    }

    void run();
};

} // namespace zrcsSystem

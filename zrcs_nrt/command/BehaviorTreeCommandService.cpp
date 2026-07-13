#include "command/BehaviorTreeCommandService.h"

#include <spdlog/spdlog.h>

#include "behavior_tree/core/BehaviorTreeService.h"

BehaviorTreeCommandService::BehaviorTreeCommandService(BehaviorTreeService* behaviorTree)
    : behaviorTree_(behaviorTree)
{
}

std::string BehaviorTreeCommandService::handle(const std::string& action,
                                               const std::string& xmlData)
{
    if (!behaviorTree_) {
        return "ERROR: BehaviorTree runner unavailable";
    }

    // 上位机行为树控制命令在此直接分发；BehaviorTreeService 只提供生命周期 API。
    if (action == "LOAD") {
        if (xmlData.empty()) {
            return "ERROR: LOAD requires xml_data";
        }
        std::string error;
        if (behaviorTree_->loadFromXml(xmlData, error)) {
            spdlog::info("[BehaviorTreeCommandService] Behavior tree loaded");
            return "OK";
        }
        spdlog::error("[BehaviorTreeCommandService] Behavior tree load failed: {}", error);
        return "ERROR: " + error;
    }

    if (action == "START") {
        std::string error;
        if (behaviorTree_->start(error)) {
            spdlog::info("[BehaviorTreeCommandService] Behavior tree started");
            return "OK";
        }
        return "ERROR: " + error;
    }

    if (action == "STOP") {
        behaviorTree_->stop("Stopped by command");
        spdlog::info("[BehaviorTreeCommandService] Behavior tree stopped");
        return "OK";
    }

    if (action == "STATUS") {
        const auto s = behaviorTree_->status();
        return "STATE=" + s.treeState + ";NODE=" + s.currentNode + ";MSG=" + s.message;
    }

    return "ERROR: Unknown BT action";
}

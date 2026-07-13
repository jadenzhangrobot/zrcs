#pragma once
/**
 * @file BehaviorTreeService.h
 * @brief 行为树生命周期与控制命令（LOAD/START/STOP/STATUS）。
 */

#include <memory>
#include <string>

#include "behavior_tree/core/BtContext.h"

class BehaviorTreeRunner;
class RtBridge;
class StatusStore;
class CommandService;
class TaskService;

class BehaviorTreeService {
public:
    struct StatusSnapshot {
        std::string treeState;
        std::string currentNode;
        std::string message;
    };

    BehaviorTreeService(RtBridge* bridge,
                        StatusStore* statusStore = nullptr,
                        CommandService* commands = nullptr,
                        TaskService* tasks = nullptr);
    ~BehaviorTreeService();

    BehaviorTreeService(const BehaviorTreeService&) = delete;
    BehaviorTreeService& operator=(const BehaviorTreeService&) = delete;

    bool loadFromXml(const std::string& xmlText, std::string& error);
    bool start(std::string& error);
    void stop(const std::string& reason = "Stopped");
    StatusSnapshot status() const;

    BehaviorTreeRunner* runner() const { return runner_.get(); }
    const BtContext& context() const { return context_; }

private:
    void publishStatusToStore();

    BtContext context_;
    std::unique_ptr<BehaviorTreeRunner> runner_;
    StatusStore* statusStore_ = nullptr;
};

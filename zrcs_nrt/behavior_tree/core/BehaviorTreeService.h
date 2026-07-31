#pragma once
/**
 * @file BehaviorTreeService.h
 * @brief 行为树生命周期与控制命令（LOAD/START/STOP/STATUS）。
 */

#include <memory>
#include <string>

#include "behavior_tree/core/BtContext.h"
#include "status/StatusTypes.h"

class BehaviorTreeRunner;
class RtBridge;
class CommandService;
class TaskService;

class BehaviorTreeService {
public:
    BehaviorTreeService(RtBridge* bridge,
                        CommandService* commands = nullptr,
                        TaskService* tasks = nullptr);
    ~BehaviorTreeService();

    BehaviorTreeService(const BehaviorTreeService&) = delete;
    BehaviorTreeService& operator=(const BehaviorTreeService&) = delete;

    bool loadFromXml(const std::string& xmlText, std::string& error);
    bool start(std::string& error);
    void stop(const std::string& reason = "Stopped");
    zrcs_nrt::BtStatus status() const;

    BehaviorTreeRunner* runner() const { return runner_.get(); }
    const BtContext& context() const { return context_; }

private:
    BtContext context_;
    std::unique_ptr<BehaviorTreeRunner> runner_;
};

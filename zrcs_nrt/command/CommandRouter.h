#pragma once
/**
 * @file CommandRouter.h
 * @brief 将外部命令路由到 CommandService / TaskService / BehaviorTreeCommandService。
 */

#include <string>
#include <vector>

class CommandService;
class TaskService;
class BehaviorTreeCommandService;

class CommandRouter {
public:
    CommandRouter(CommandService* commands,
                  TaskService* tasks,
                  BehaviorTreeCommandService* behaviorTreeCommands);

    /// MotionCommand：name + args → Task 或 RT 命令。
    std::string routeMotion(const std::string& name, const std::vector<double>& args);

    /// BehaviorTreeCommand：action + 可选 xml。
    std::string routeBehaviorTree(const std::string& action, const std::string& xmlData);

private:
    CommandService* commands_;
    TaskService* tasks_;
    BehaviorTreeCommandService* behaviorTreeCommands_;
};

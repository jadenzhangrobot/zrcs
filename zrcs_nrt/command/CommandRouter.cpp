#include "command/CommandRouter.h"

#include "command/BehaviorTreeCommandService.h"
#include "command/CommandService.h"
#include "command/TaskService.h"

CommandRouter::CommandRouter(CommandService* commands,
                             TaskService* tasks,
                             BehaviorTreeCommandService* behaviorTreeCommands)
    : commands_(commands)
    , tasks_(tasks)
    , behaviorTreeCommands_(behaviorTreeCommands)
{
}

std::string CommandRouter::routeMotion(const std::string& name, const std::vector<double>& args)
{
    std::string reply;
    if (tasks_ && tasks_->tryHandle(name, args, reply)) {
        return reply;
    }
    if (!commands_) {
        return "ERROR: Not connected";
    }
    return commands_->submit(name, args);
}

std::string CommandRouter::routeBehaviorTree(const std::string& action,
                                             const std::string& xmlData)
{
    if (!behaviorTreeCommands_) {
        return "ERROR: BehaviorTree runner unavailable";
    }
    return behaviorTreeCommands_->handle(action, xmlData);
}

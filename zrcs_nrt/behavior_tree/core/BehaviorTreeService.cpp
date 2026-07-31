#include "behavior_tree/core/BehaviorTreeService.h"

#include <spdlog/spdlog.h>

#include "behavior_tree/core/BehaviorTreeRunner.h"

BehaviorTreeService::BehaviorTreeService(RtBridge* bridge,
                                         CommandService* commands,
                                         TaskService* tasks)
{
    context_.bridge = bridge;
    context_.commands = commands;
    context_.tasks = tasks;
    context_.sharedState = std::make_shared<zrcs_bt::SharedState>();
    context_.sharedState->bridge = bridge;

    runner_ = std::make_unique<BehaviorTreeRunner>(context_);
}

BehaviorTreeService::~BehaviorTreeService() = default;

bool BehaviorTreeService::loadFromXml(const std::string& xmlText, std::string& error)
{
    return runner_->loadFromXml(xmlText, error);
}

bool BehaviorTreeService::start(std::string& error)
{
    return runner_->start(error);
}

void BehaviorTreeService::stop(const std::string& reason)
{
    runner_->stop(reason);
}

zrcs_nrt::BtStatus BehaviorTreeService::status() const
{
    return runner_->status();
}

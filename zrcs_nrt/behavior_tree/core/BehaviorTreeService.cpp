#include "behavior_tree/core/BehaviorTreeService.h"

#include <spdlog/spdlog.h>

#include "behavior_tree/core/BehaviorTreeRunner.h"
#include "status/StatusStore.h"
#include "status/StatusTypes.h"

BehaviorTreeService::BehaviorTreeService(RtBridge* bridge,
                                         StatusStore* statusStore,
                                         CommandService* commands,
                                         TaskService* tasks)
    : statusStore_(statusStore)
{
    context_.bridge = bridge;
    context_.status = statusStore;
    context_.commands = commands;
    context_.tasks = tasks;
    context_.sharedState = std::make_shared<zrcs_bt::SharedState>();
    context_.sharedState->bridge = bridge;

    runner_ = std::make_unique<BehaviorTreeRunner>(context_);
}

BehaviorTreeService::~BehaviorTreeService() = default;

void BehaviorTreeService::publishStatusToStore()
{
    if (!statusStore_) {
        return;
    }
    const auto s = status();
    zrcs_nrt::BtStatusSnapshot snap;
    snap.treeState = s.treeState;
    snap.currentNode = s.currentNode;
    snap.message = s.message;
    statusStore_->setBtStatus(snap);
}

bool BehaviorTreeService::loadFromXml(const std::string& xmlText, std::string& error)
{
    const bool ok = runner_->loadFromXml(xmlText, error);
    publishStatusToStore();
    return ok;
}

bool BehaviorTreeService::start(std::string& error)
{
    const bool ok = runner_->start(error);
    publishStatusToStore();
    return ok;
}

void BehaviorTreeService::stop(const std::string& reason)
{
    runner_->stop(reason);
    publishStatusToStore();
}

BehaviorTreeService::StatusSnapshot BehaviorTreeService::status() const
{
    const auto s = runner_->status();
    return StatusSnapshot{s.treeState, s.currentNode, s.message};
}

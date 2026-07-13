#pragma once
/**
 * @file BtContext.h
 * @brief 行为树节点依赖注入（非 owning）。
 */

#include <memory>

#include "behavior_tree/core/BtSharedState.h"

class CommandService;
class TaskService;
class StatusStore;
class RtBridge;

struct BtContext {
    CommandService* commands = nullptr;
    TaskService* tasks = nullptr;
    StatusStore* status = nullptr;
    RtBridge* bridge = nullptr; ///< legacy 节点仍通过 SharedState/bridge 发令
    std::shared_ptr<zrcs_bt::SharedState> sharedState;
};

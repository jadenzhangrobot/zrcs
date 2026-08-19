#pragma once
/**
 * @file BtContext.h
 * @brief 行为树节点依赖注入（非 owning）。
 */

#include <memory>

#include "behavior_tree/core/BtSharedState.h"

class CommandService;
class TaskService;
class RtBridge;

struct BtContext {
    CommandService* commands = nullptr;
    TaskService* tasks = nullptr;
    RtBridge* bridge = nullptr; ///< 仅用于查询（isCommandCompleted 等），发令请走 commands
    std::shared_ptr<zrcs_bt::SharedState> sharedState;
};

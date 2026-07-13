#pragma once
/**
 * @file RegisterNodes.h
 * @brief 行为树节点统一注册入口（core 唯一入口）。
 */

struct BtContext;
class BtNodeRegistry;

/// 注册全部行为树节点（core 命令节点 + 运动节点等）。
void registerAllBehaviorTreeNodes(BtNodeRegistry& registry, const BtContext& context);

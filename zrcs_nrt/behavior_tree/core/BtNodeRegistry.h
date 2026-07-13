#pragma once
/**
 * @file BtNodeRegistry.h
 * @brief 对 BehaviorTreeFactory 注册的薄封装，避免 Runner 内堆砌节点清单。
 */

#include <behaviortree_cpp_v3/bt_factory.h>

class BtNodeRegistry {
public:
    explicit BtNodeRegistry(BT::BehaviorTreeFactory& factory);

    BT::BehaviorTreeFactory& factory() { return factory_; }
    const BT::BehaviorTreeFactory& factory() const { return factory_; }

private:
    BT::BehaviorTreeFactory& factory_;
};

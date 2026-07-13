#pragma once
/**
 * @file BehaviorTreeCommandService.h
 * @brief 上位机下发的行为树控制命令入口（LOAD / START / STOP / STATUS）。
 *
 * 与 CommandService / TaskService 并列，由 CommandRouter 路由。
 * 在本类中完成 action 分发与回复串；BehaviorTreeService 只负责生命周期实现。
 */

#include <string>

class BehaviorTreeService;

class BehaviorTreeCommandService {
public:
    explicit BehaviorTreeCommandService(BehaviorTreeService* behaviorTree);

    /// 处理上位机 BT 控制命令，返回 "OK" / "ERROR: ..." / STATUS 文本。
    std::string handle(const std::string& action, const std::string& xmlData);

    bool available() const { return behaviorTree_ != nullptr; }

private:
    BehaviorTreeService* behaviorTree_;
};

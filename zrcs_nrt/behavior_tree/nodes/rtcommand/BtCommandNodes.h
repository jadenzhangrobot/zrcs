#pragma once
/**
 * @file BtCommandNodes.h
 * @brief RT 命令类行为树节点（SendCommand / BatchCmd / 各命令别名）。
 *
 * 包含两类命令节点：
 *   - SendCommandNode      通用的 CSV args 字符串节点（兼容旧 XML）
 *   - TypedSendCommandNode<ArgEnum>  按 Arg 枚举成员生成独立命名输入端口的模板节点
 */

#include <memory>
#include <string>
#include <utility>

#include <behaviortree_cpp_v3/bt_factory.h>

#include "behavior_tree/core/BtSharedState.h"
#include "rtBridge/RtBridge.h"
#include "config/CmdDefine.h"

namespace zrcs_bt {

// ============================================================================
// SendCommandNode — 通用 CSV args 节点（兼容旧 XML）
// ============================================================================

class SendCommandNode : public BT::StatefulActionNode
{
public:
    SendCommandNode(const std::string& name,
                    const BT::NodeConfiguration& config,
                    std::shared_ptr<SharedState> sharedState,
                    std::string fixedCommandName = {});

    static BT::PortsList providedPorts();
    static BT::PortsList aliasPorts();

    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    std::shared_ptr<SharedState> sharedState_;
    std::string fixedCommandName_;
    uint32_t pendingSeq_{0};
};

// ============================================================================
// TypedSendCommandNode<ArgEnum> — 模板节点，实现必须留在头文件
// ============================================================================

template <typename ArgEnum>
class TypedSendCommandNode : public BT::StatefulActionNode
{
public:
    TypedSendCommandNode(const std::string& name,
                         const BT::NodeConfiguration& config,
                         std::shared_ptr<SharedState> sharedState,
                         std::string commandName)
        : BT::StatefulActionNode(name, config)
        , sharedState_(std::move(sharedState))
        , commandName_(std::move(commandName))
    {}

    template <std::size_t... Is>
    static BT::PortsList providedPortsImpl(std::index_sequence<Is...>)
    {
        constexpr auto names = magic_enum::enum_names<ArgEnum>();
        return {BT::InputPort<double>(std::string(names[Is]), 0.0, "")...};
    }

    static BT::PortsList providedPorts()
    {
        if constexpr (!std::is_same_v<ArgEnum, void>)
            return providedPortsImpl(std::make_index_sequence<magic_enum::enum_count<ArgEnum>()>());
        return {};
    }

    BT::NodeStatus onStart() override
    {
        double args[zrcs::kCmdArgsMax]{};
        std::size_t count = 0;

        if constexpr (!std::is_same_v<ArgEnum, void>)
        {
            constexpr auto names = magic_enum::enum_names<ArgEnum>();
            for (std::size_t i = 0; i < names.size(); ++i)
            {
                auto opt = getInput<double>(std::string(names[i]));
                args[i] = opt.has_value() ? *opt : 0.0;
            }
            count = names.size();
        }

        sharedState_->setCurrentNode(name(), "send command=" + commandName_);

        const auto [result, seq] = sharedState_->bridge->sendCommand(
            commandName_, args, count);

        if (result != RtBridge::SendResult::OK)
        {
            const std::string msg =
                "sendCommand failed: " + std::to_string(static_cast<int>(result));
            sharedState_->setCurrentNode(name(), msg);
            return BT::NodeStatus::FAILURE;
        }

        pendingSeq_ = seq;
        sharedState_->setCurrentNode(name(), "waiting seq=" + std::to_string(seq));
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        if (!sharedState_->bridge->isCommandCompleted(pendingSeq_))
            return BT::NodeStatus::RUNNING;

        const auto completion = sharedState_->bridge->lastCompletion();
        const bool success = completion.success;
        if (success)
        {
            sharedState_->setCurrentNode(name(), "completed seq=" + std::to_string(pendingSeq_));
            return BT::NodeStatus::SUCCESS;
        }

        sharedState_->setCurrentNode(name(), "failed seq=" + std::to_string(pendingSeq_));
        return BT::NodeStatus::FAILURE;
    }

    void onHalted() override
    {
        sharedState_->bridge->requestStop();
        sharedState_->setCurrentNode(name(), "halted");
    }

private:
    std::shared_ptr<SharedState> sharedState_;
    std::string commandName_;
    uint32_t pendingSeq_{0};
};

// ============================================================================
// BatchCmdNode — 批量命令节点
// ============================================================================

class BatchCmdNode : public BT::StatefulActionNode
{
public:
    BatchCmdNode(const std::string& name,
                 const BT::NodeConfiguration& config,
                 std::shared_ptr<SharedState> sharedState);

    static BT::PortsList providedPorts();

    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    std::shared_ptr<SharedState> sharedState_;
    uint32_t pendingSeq_{0};
};

// ============================================================================
// ButterflyPathNode — 蝴蝶路径演示节点
// ============================================================================

} // namespace zrcs_bt

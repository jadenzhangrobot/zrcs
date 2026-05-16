#pragma once
/**
 * @file BtCommandNodes.h
 * @brief 行为树命令节点类型。
 *
 * 包含两类命令节点：
 *   - SendCommandNode      通用的 CSV args 字符串节点（兼容旧 XML）
 *   - TypedSendCommandNode<ArgEnum>  按 Arg 枚举成员生成独立命名输入端口的模板节点
 *
 * 两者均通过 SharedState 桥接 RtBridge，将命令推入 RT 共享内存队列，
 * 并轮询 lastCmdSeq 等待命令完成。
 */

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include <behaviortree_cpp_v3/bt_factory.h>
#include <spdlog/spdlog.h>

#include "config/CmdDefine.h"   // CmdId, Arg 枚举, magic_enum, kCmdArgsMax
#include "rtBridge/RtBridge.h"  // RtBridge, SendResult
#include "BtSharedState.h"

namespace zrcs_bt {

// ============================================================================
// SendCommandNode — 通用 CSV args 节点（兼容旧 XML）
// ============================================================================

/**
 * @brief 通过 CSV 字符串参数发送 RT 命令的 BehaviorTree 节点。
 *
 * 输入端 :
 *   - "command" (std::string) : 命令名，如 "MoveJ"。别名节点已预填，无需手动指定。
 *   - "args"    (std::string) : 逗号分隔的参数值字符串。
 *
 * 生命周期 : onStart() 发送命令 → onRunning() 轮询完成 → SUCCESS / FAILURE
 */
class SendCommandNode : public BT::StatefulActionNode
{
public:
    SendCommandNode(const std::string& name,
                    const BT::NodeConfiguration& config,
                    std::shared_ptr<SharedState> sharedState,
                    std::string fixedCommandName = {})
        : BT::StatefulActionNode(name, config)
        , sharedState_(std::move(sharedState))
        , fixedCommandName_(std::move(fixedCommandName))
    {}

    /// 通用节点端口 : command + args。
    static BT::PortsList providedPorts()
    {
        return
        {
            BT::InputPort<std::string>("command"),
            BT::InputPort<std::string>("args", "")
        };
    }

    /// 别名节点端口 : 仅 args（command 已由构造函数固定）。
    static BT::PortsList aliasPorts()
    {
        return
        {
            BT::InputPort<std::string>("args", "")
        };
    }

    BT::NodeStatus onStart() override
    {
        std::string commandName = fixedCommandName_;
        if (commandName.empty())
        {
            auto command = getInput<std::string>("command");
            if (!command)
            {
                throw BT::RuntimeError("missing required input [command]: ", command.error());
            }
            commandName = *command;
        }

        const auto args = getInput<std::string>("args");
        const std::string csvArgs = args ? *args : std::string();

        sharedState_->setCurrentNode(name(), "send command=" + commandName);

        const auto [result, seq] = sharedState_->bridge->sendCommand(commandName, csvArgs);
        if (result != RtBridge::SendResult::OK)
        {
            std::ostringstream oss;
            oss << "sendCommand failed: " << static_cast<int>(result);
            sharedState_->setCurrentNode(name(), oss.str());
            spdlog::error("[BehaviorTree] {}", oss.str());
            return BT::NodeStatus::FAILURE;
        }

        pendingSeq_ = seq;
        sharedState_->setCurrentNode(name(), "waiting seq=" + std::to_string(seq));
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        if (!sharedState_->bridge->isCommandCompleted(pendingSeq_))
        {
            return BT::NodeStatus::RUNNING;
        }

        const auto completion = sharedState_->bridge->lastCompletion();
        const bool success = completion.seq == pendingSeq_ ? completion.success : completion.success;
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
    std::string fixedCommandName_;
    uint32_t pendingSeq_{0};
};

// ============================================================================
// TypedSendCommandNode<ArgEnum> — 按 Arg 枚举生成命名输入端口的模板节点
// ============================================================================

/**
 * @brief 按 Arg 枚举成员自动生成独立 double 输入端口的命令节点。
 *
 * @tparam ArgEnum 对应命令的 Arg 枚举（如 JogabsJArg）。void 表示无参数命令。
 *
 * 每个 ArgEnum 成员自动生成一个 InputPort<double>，端口名与枚举成员名一致。
 * onStart() 中按枚举顺序读取各端口，组装为 double[] 后推入 RT 命令队列。
 */
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

    // -- 端口生成（编译期，利用 index_sequence 展开 magic_enum 枚举名）--------

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

    // -- 生命周期 ---------------------------------------------------------------

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
            std::ostringstream oss;
            oss << "sendCommand failed: " << static_cast<int>(result);
            sharedState_->setCurrentNode(name(), oss.str());
            spdlog::error("[BehaviorTree] {}", oss.str());
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
        const bool success = completion.seq == pendingSeq_ ? completion.success : completion.success;
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
// BatchCmdNode — 批量命令节点，一次性发送多段到 RT 队列实现无缝衔接
// ============================================================================

/**
 * @brief 批量发送同类型命令的节点。
 *
 * 输入端 :
 *   - "command"  (std::string) : 命令名，如 "MoveL"、"MoveLGalvo"
 *   - "segments" (std::string) : | 分隔的多段参数，每段为逗号分隔的 double 值
 *
 * onStart() 中一次性将所有段推入 RT 命令队列，仅轮询最后一段的完成状态。
 * 适用于测试多段轨迹的段间连续性。
 */
class BatchCmdNode : public BT::StatefulActionNode
{
public:
    BatchCmdNode(const std::string& name,
                 const BT::NodeConfiguration& config,
                 std::shared_ptr<SharedState> sharedState)
        : BT::StatefulActionNode(name, config)
        , sharedState_(std::move(sharedState))
    {}

    static BT::PortsList providedPorts()
    {
        return
        {
            BT::InputPort<std::string>("command"),
            BT::InputPort<std::string>("segments")
        };
    }

    BT::NodeStatus onStart() override
    {
        auto cmdOpt = getInput<std::string>("command");
        if (!cmdOpt || cmdOpt->empty())
        {
            throw BT::RuntimeError("BatchCmd: missing required input [command]");
        }
        const std::string& commandName = *cmdOpt;

        auto segOpt = getInput<std::string>("segments");
        if (!segOpt || segOpt->empty())
        {
            throw BT::RuntimeError("BatchCmd: missing required input [segments]");
        }
        const std::string& segStr = *segOpt;

        // 按 | 拆分各段
        std::vector<std::string> segParts;
        {
            std::istringstream iss(segStr);
            std::string part;
            while (std::getline(iss, part, '|'))
            {
                // 去除首尾空白
                size_t b = 0, e = part.size();
                while (b < e && std::isspace(static_cast<unsigned char>(part[b]))) ++b;
                while (e > b && std::isspace(static_cast<unsigned char>(part[e - 1]))) --e;
                if (b < e)
                    segParts.emplace_back(part.substr(b, e - b));
            }
        }

        if (segParts.empty())
        {
            throw BT::RuntimeError("BatchCmd: segments is empty");
        }

        // 逐段发送，记录最后一条的 seq
        uint32_t lastSeq = 0;
        for (const auto& seg : segParts)
        {
            std::vector<double> args;
            {
                std::istringstream iss(seg);
                std::string token;
                while (std::getline(iss, token, ','))
                {
                    args.push_back(std::stod(token));
                }
            }

            if (args.empty()) continue;

            const auto [result, seq] = sharedState_->bridge->sendCommand(
                commandName, args.data(), args.size());

            if (result != RtBridge::SendResult::OK)
            {
                std::ostringstream oss;
                oss << "BatchCmd sendCommand failed: " << static_cast<int>(result);
                sharedState_->setCurrentNode(name(), oss.str());
                spdlog::error("[BehaviorTree] {}", oss.str());
                return BT::NodeStatus::FAILURE;
            }
            lastSeq = seq;
        }

        pendingSeq_ = lastSeq;
        sharedState_->setCurrentNode(name(),
            "batch sent " + std::to_string(segParts.size()) + " segments, waiting seq=" + std::to_string(lastSeq));
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        if (!sharedState_->bridge->isCommandCompleted(pendingSeq_))
            return BT::NodeStatus::RUNNING;

        const auto completion = sharedState_->bridge->lastCompletion();
        const bool success = completion.seq == pendingSeq_ ? completion.success : completion.success;
        if (success)
        {
            sharedState_->setCurrentNode(name(), "batch completed seq=" + std::to_string(pendingSeq_));
            return BT::NodeStatus::SUCCESS;
        }

        sharedState_->setCurrentNode(name(), "batch failed seq=" + std::to_string(pendingSeq_));
        return BT::NodeStatus::FAILURE;
    }

    void onHalted() override
    {
        sharedState_->bridge->requestStop();
        sharedState_->setCurrentNode(name(), "halted");
    }

private:
    std::shared_ptr<SharedState> sharedState_;
    uint32_t pendingSeq_{0};
};

} // namespace zrcs_bt

#include "behavior_tree/nodes/rtcommand/BtCommandNodes.h"

#include <cctype>
#include <sstream>
#include <vector>

#include <spdlog/spdlog.h>

#include "algorithm/path_planning/TrajectoryTypes.h"

namespace zrcs_bt {

// ---------------------------------------------------------------------------
// SendCommandNode
// ---------------------------------------------------------------------------

SendCommandNode::SendCommandNode(const std::string& name,
                                 const BT::NodeConfiguration& config,
                                 std::shared_ptr<SharedState> sharedState,
                                 std::string fixedCommandName)
    : BT::StatefulActionNode(name, config)
    , sharedState_(std::move(sharedState))
    , fixedCommandName_(std::move(fixedCommandName))
{
}

BT::PortsList SendCommandNode::providedPorts()
{
    return {
        BT::InputPort<std::string>("command"),
        BT::InputPort<std::string>("args", "")
    };
}

BT::PortsList SendCommandNode::aliasPorts()
{
    return {
        BT::InputPort<std::string>("args", "")
    };
}

BT::NodeStatus SendCommandNode::onStart()
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

BT::NodeStatus SendCommandNode::onRunning()
{
    if (!sharedState_->bridge->isCommandCompleted(pendingSeq_))
    {
        return BT::NodeStatus::RUNNING;
    }

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

void SendCommandNode::onHalted()
{
    sharedState_->bridge->requestStop();
    sharedState_->setCurrentNode(name(), "halted");
}

// ---------------------------------------------------------------------------
// BatchCmdNode
// ---------------------------------------------------------------------------

BatchCmdNode::BatchCmdNode(const std::string& name,
                           const BT::NodeConfiguration& config,
                           std::shared_ptr<SharedState> sharedState)
    : BT::StatefulActionNode(name, config)
    , sharedState_(std::move(sharedState))
{
}

BT::PortsList BatchCmdNode::providedPorts()
{
    return {
        BT::InputPort<std::string>("command"),
        BT::InputPort<std::string>("segments")
    };
}

BT::NodeStatus BatchCmdNode::onStart()
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

    std::vector<std::string> segParts;
    {
        std::istringstream iss(segStr);
        std::string part;
        while (std::getline(iss, part, '|'))
        {
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

    uint32_t lastSeq = 0;
    for (size_t segIdx = 0; segIdx < segParts.size(); ++segIdx)
    {
        const auto& seg = segParts[segIdx];
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

        // 第一段设 Sync=1 重置弧长参数空间
        args.push_back(segIdx == 0 ? 1.0 : 0.0);

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
    sharedState_->setCurrentNode(
        name(),
        "batch sent " + std::to_string(segParts.size()) +
            " segments, waiting seq=" + std::to_string(lastSeq));
    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus BatchCmdNode::onRunning()
{
    if (!sharedState_->bridge->isCommandCompleted(pendingSeq_))
        return BT::NodeStatus::RUNNING;

    const auto completion = sharedState_->bridge->lastCompletion();
    const bool success = completion.success;
    if (success)
    {
        sharedState_->setCurrentNode(name(), "batch completed seq=" + std::to_string(pendingSeq_));
        return BT::NodeStatus::SUCCESS;
    }

    sharedState_->setCurrentNode(name(), "batch failed seq=" + std::to_string(pendingSeq_));
    return BT::NodeStatus::FAILURE;
}

void BatchCmdNode::onHalted()
{
    sharedState_->bridge->requestStop();
    sharedState_->setCurrentNode(name(), "halted");
}

} // namespace zrcs_bt

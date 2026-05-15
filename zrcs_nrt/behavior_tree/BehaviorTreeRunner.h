#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include <behaviortree_cpp_v3/bt_factory.h>
#include <spdlog/spdlog.h>

#include "rtBridge/RtBridge.h"

class BehaviorTreeRunner;

namespace zrcs_bt {

struct SharedState {
    RtBridge* bridge = nullptr;
    mutable std::mutex mutex;
    std::string currentNode;
    std::string message;

    void setCurrentNode(const std::string& nodeName, const std::string& text)
    {
        std::lock_guard<std::mutex> lock(mutex);
        currentNode = nodeName;
        message = text;
    }

    std::pair<std::string, std::string> snapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return {currentNode, message};
    }
};

class LogMessageNode : public BT::SyncActionNode 
{
public:
    LogMessageNode(const std::string& name,
                   const BT::NodeConfiguration& config,
                   std::shared_ptr<SharedState> sharedState)
        : BT::SyncActionNode(name, config)
        , sharedState_(std::move(sharedState))
    {
    }

    static BT::PortsList providedPorts()
    {
        return {BT::InputPort<std::string>("message")};
    }

    BT::NodeStatus tick() override
    {
        auto message = getInput<std::string>("message");
        if (!message)
        {
            throw BT::RuntimeError("missing required input [message]: ", message.error());
        }

        sharedState_->setCurrentNode(name(), *message);
        spdlog::info("[BehaviorTree] {}", *message);
        return BT::NodeStatus::SUCCESS;
    }

private:
    std::shared_ptr<SharedState> sharedState_;
};

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
    {
    }

    static BT::PortsList providedPorts()
    {
        return 
        {
            BT::InputPort<std::string>("command"),
            BT::InputPort<std::string>("args", "")
        };
    }

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

} // namespace zrcs_bt

class BehaviorTreeRunner {
public:
    struct StatusSnapshot {
        std::string treeState;
        std::string currentNode;
        std::string message;
    };

    explicit BehaviorTreeRunner(RtBridge* bridge)
        : bridge_(bridge)
        , sharedState_(std::make_shared<zrcs_bt::SharedState>())
    {
        sharedState_->bridge = bridge_;
        registerNodes();
        setStatus("IDLE", "", "No tree loaded");
    }

    ~BehaviorTreeRunner()
    {
        stop("Runner destroyed");
    }

    bool loadFromXml(const std::string& xmlText, std::string& error)
    {
        stop("Reload tree");

        try
        {
            auto tree = std::make_unique<BT::Tree>(factory_.createTreeFromText(xmlText));
            {
                std::lock_guard<std::mutex> lock(mutex_);
                tree_ = std::move(tree);
            }
            setStatus("LOADED", "", "Tree loaded");
            return true;
        }
        catch (const std::exception& e)
        {
            error = e.what();
            setStatus("FAILURE", "", error);
            return false;
        }
    }

    bool start(std::string& error)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!tree_)
        {
            error = "No tree loaded";
            return false;
        }
        if (running_.load(std::memory_order_acquire))
        {
            error = "Tree already running";
            return false;
        }

        running_.store(true, std::memory_order_release);
        tickThread_ = std::thread(&BehaviorTreeRunner::tickLoop, this);
        setStatus("RUNNING", "", "Tree started");
        return true;
    }

    void stop(const std::string& reason = "Stopped")
    {
        running_.store(false, std::memory_order_release);

        std::unique_lock<std::mutex> lock(mutex_);
        if (tree_)
        {
            tree_->haltTree();
        }
        lock.unlock();

        if (tickThread_.joinable())
        {
            tickThread_.join();
        }

        const auto current = status();
        if (current.treeState == "RUNNING" || current.treeState == "LOADED")
        {
            setStatus("HALTED", current.currentNode, reason);
        }
    }

    StatusSnapshot status() const
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        return status_;
    }

private:
    void registerNodes()
    {
        factory_.registerBuilder<zrcs_bt::LogMessageNode>(
            "LogMessage",
            [sharedState = sharedState_](const std::string& name, const BT::NodeConfiguration& config)
            {
                return std::make_unique<zrcs_bt::LogMessageNode>(name, config, sharedState);
            });

        factory_.registerBuilder<zrcs_bt::SendCommandNode>(
            "SendCommand",
            [sharedState = sharedState_](const std::string& name, const BT::NodeConfiguration& config)
            {
                return std::make_unique<zrcs_bt::SendCommandNode>(name, config, sharedState);
            });

        registerCommandAliasNodes();
    }

    void registerCommandAliasNodes()
    {
        for (int value = Enable; value < SENTINEL; ++value)
        {
            const auto commandId = static_cast<CmdId>(value);
            const std::string commandName = zrcs::cmdIdToName(commandId);
            const BT::TreeNodeManifest manifest{
                BT::NodeType::ACTION,
                commandName,
                zrcs_bt::SendCommandNode::aliasPorts(),
                "ZRCS RT command"
            };

            factory_.registerBuilder(
                manifest,
                [sharedState = sharedState_, commandName](const std::string& name, const BT::NodeConfiguration& config)
                {
                    return std::make_unique<zrcs_bt::SendCommandNode>(name, config, sharedState, commandName);
                });
        }
    }

    void tickLoop()
    {
        while (running_.load(std::memory_order_acquire))
        {
            BT::NodeStatus nodeStatus = BT::NodeStatus::RUNNING;
            try
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (!tree_)
                {
                    setStatus("FAILURE", "", "Tree disappeared while running");
                    running_.store(false, std::memory_order_release);
                    return;
                }
                nodeStatus = tree_->tickRoot();
            }
            catch (const std::exception& e)
            {
                setStatus("FAILURE", "", e.what());
                running_.store(false, std::memory_order_release);
                return;
            }

            const auto snapshot = sharedState_->snapshot();
            if (nodeStatus == BT::NodeStatus::RUNNING)
            {
                setStatus("RUNNING", snapshot.first, snapshot.second);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            if (nodeStatus == BT::NodeStatus::SUCCESS)
            {
                setStatus("SUCCESS", snapshot.first, snapshot.second.empty() ? "Tree completed" : snapshot.second);
            }
            else
            {
                setStatus("FAILURE", snapshot.first, snapshot.second.empty() ? "Tree failed" : snapshot.second);
            }
            running_.store(false, std::memory_order_release);
            return;
        }
    }

    void setStatus(const std::string& treeState,
                   const std::string& currentNode,
                   const std::string& message)
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_.treeState = treeState;
        status_.currentNode = currentNode;
        status_.message = message;
    }

    RtBridge* bridge_;
    BT::BehaviorTreeFactory factory_;
    std::shared_ptr<zrcs_bt::SharedState> sharedState_;
    std::unique_ptr<BT::Tree> tree_;
    std::atomic<bool> running_{false};
    std::thread tickThread_;
    mutable std::mutex mutex_;
    mutable std::mutex statusMutex_;
    StatusSnapshot status_;
};
#include "behaviorTree/BehaviorTreeRunner.h"

#include <chrono>

#include "command/RtBridge.h"
#include "config/CmdDefine.h"

BehaviorTreeRunner::BehaviorTreeRunner(RtBridge* bridge)
    : bridge_(bridge)
    , sharedState_(std::make_shared<zrcs_bt::SharedState>())
{
    sharedState_->bridge = bridge_;
    registerNodes();
    setStatus("IDLE", "", "No tree loaded");
}

BehaviorTreeRunner::~BehaviorTreeRunner()
{
    stop("Runner destroyed");
}

bool BehaviorTreeRunner::loadFromXml(const std::string& xmlText, std::string& error)
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

bool BehaviorTreeRunner::start(std::string& error)
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

    if (tickThread_.joinable())
        tickThread_.join();
    running_.store(true, std::memory_order_release);
    tickThread_ = std::thread(&BehaviorTreeRunner::tickLoop, this);
    setStatus("RUNNING", "", "Tree started");
    return true;
}

void BehaviorTreeRunner::stop(const std::string& reason)
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

BehaviorTreeRunner::StatusSnapshot BehaviorTreeRunner::status() const
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return status_;
}

void BehaviorTreeRunner::registerNodes()
{
    factory_.registerBuilder<zrcs_bt::SendCommandNode>(
        "SendCommand",
        [sharedState = sharedState_](const std::string& name, const BT::NodeConfiguration& config)
        {
            return std::make_unique<zrcs_bt::SendCommandNode>(name, config, sharedState);
        });

    factory_.registerBuilder<zrcs_bt::BatchCmdNode>(
        "BatchCmd",
        [sharedState = sharedState_](const std::string& name, const BT::NodeConfiguration& config)
        {
            return std::make_unique<zrcs_bt::BatchCmdNode>(name, config, sharedState);
        });

    factory_.registerBuilder<zrcs_bt::ButterflyPathNode>(
        "ButterflyPath",
        [sharedState = sharedState_](const std::string& name, const BT::NodeConfiguration& config)
        {
            return std::make_unique<zrcs_bt::ButterflyPathNode>(name, config, sharedState);
        });

    registerCommandAliasNodes();
}

void BehaviorTreeRunner::registerCommandAliasNodes()
{
#define ZRCS_REGISTER_ALIAS(CmdName, ArgType) \
    registerTypedAlias<ArgType>(zrcs::cmdIdToName(CmdId::CmdName));
    ZRCS_MOTION_COMMAND_TABLE(ZRCS_REGISTER_ALIAS)
#undef ZRCS_REGISTER_ALIAS
}

void BehaviorTreeRunner::tickLoop()
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
            setStatus("SUCCESS", snapshot.first,
                      snapshot.second.empty() ? "Tree completed" : snapshot.second);
        }
        else
        {
            setStatus("FAILURE", snapshot.first,
                      snapshot.second.empty() ? "Tree failed" : snapshot.second);
        }
        running_.store(false, std::memory_order_release);
        return;
    }
}

void BehaviorTreeRunner::setStatus(const std::string& treeState,
                                   const std::string& currentNode,
                                   const std::string& message)
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    status_.treeState = treeState;
    status_.currentNode = currentNode;
    status_.message = message;
}

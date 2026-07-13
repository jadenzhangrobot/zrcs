#include "behavior_tree/core/BehaviorTreeRunner.h"

#include <chrono>

#include "behavior_tree/core/BtNodeRegistry.h"
#include "behavior_tree/core/RegisterNodes.h"
#include "rtBridge/RtBridge.h"

BehaviorTreeRunner::BehaviorTreeRunner(const BtContext& context)
    : context_(context)
{
    if (!context_.sharedState) {
        context_.sharedState = std::make_shared<zrcs_bt::SharedState>();
    }
    if (context_.bridge) {
        context_.sharedState->bridge = context_.bridge;
    }
    registerAllNodes();
    setStatus("IDLE", "", "No tree loaded");
}

BehaviorTreeRunner::BehaviorTreeRunner(RtBridge* bridge)
    : BehaviorTreeRunner(BtContext{nullptr, nullptr, nullptr, bridge, nullptr})
{
}

BehaviorTreeRunner::~BehaviorTreeRunner()
{
    stop("Runner destroyed");
}

void BehaviorTreeRunner::registerAllNodes()
{
    BtNodeRegistry registry(factory_);
    registerAllBehaviorTreeNodes(registry, context_);
}

bool BehaviorTreeRunner::loadFromXml(const std::string& xmlText, std::string& error)
{
    stop("Reload tree");

    try {
        auto tree = std::make_unique<BT::Tree>(factory_.createTreeFromText(xmlText));
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tree_ = std::move(tree);
        }
        setStatus("LOADED", "", "Tree loaded");
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        setStatus("FAILURE", "", error);
        return false;
    }
}

bool BehaviorTreeRunner::start(std::string& error)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!tree_) {
        error = "No tree loaded";
        return false;
    }
    if (running_.load(std::memory_order_acquire)) {
        error = "Tree already running";
        return false;
    }

    if (tickThread_.joinable()) {
        tickThread_.join();
    }
    running_.store(true, std::memory_order_release);
    tickThread_ = std::thread(&BehaviorTreeRunner::tickLoop, this);
    setStatus("RUNNING", "", "Tree started");
    return true;
}

void BehaviorTreeRunner::stop(const std::string& reason)
{
    running_.store(false, std::memory_order_release);

    std::unique_lock<std::mutex> lock(mutex_);
    if (tree_) {
        tree_->haltTree();
    }
    lock.unlock();

    if (tickThread_.joinable()) {
        tickThread_.join();
    }

    const auto current = status();
    if (current.treeState == "RUNNING" || current.treeState == "LOADED") {
        setStatus("HALTED", current.currentNode, reason);
    }
}

BehaviorTreeRunner::StatusSnapshot BehaviorTreeRunner::status() const
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return status_;
}

void BehaviorTreeRunner::tickLoop()
{
    while (running_.load(std::memory_order_acquire)) {
        BT::NodeStatus nodeStatus = BT::NodeStatus::RUNNING;
        try {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!tree_) {
                setStatus("FAILURE", "", "Tree disappeared while running");
                running_.store(false, std::memory_order_release);
                return;
            }
            nodeStatus = tree_->tickRoot();
        } catch (const std::exception& e) {
            setStatus("FAILURE", "", e.what());
            running_.store(false, std::memory_order_release);
            return;
        }

        const auto snapshot = context_.sharedState->snapshot();
        if (nodeStatus == BT::NodeStatus::RUNNING) {
            setStatus("RUNNING", snapshot.first, snapshot.second);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        if (nodeStatus == BT::NodeStatus::SUCCESS) {
            setStatus("SUCCESS",
                      snapshot.first,
                      snapshot.second.empty() ? "Tree completed" : snapshot.second);
        } else {
            setStatus("FAILURE",
                      snapshot.first,
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

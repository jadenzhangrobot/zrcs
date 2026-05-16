#pragma once
/**
 * @file BehaviorTreeRunner.h
 * @brief 行为树生命周期管理器。
 *
 * 负责加载 / 启动 / 停止 BehaviorTree XML，管理 tick 线程，
 * 并注册所有可用的 RT 命令节点（SendCommandNode + TypedSendCommandNode）。
 *
 * 使用方式 :
 *   1. loadFromXml(xmlText)  加载 XML 并编译为 BT::Tree
 *   2. start()               启动 tick 线程开始执行
 *   3. stop()                停止执行并 join 线程
 *   4. status()              获取当前运行状态快照
 */

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <behaviortree_cpp_v3/bt_factory.h>

#include "BtCommandNodes.h"  // SendCommandNode, TypedSendCommandNode<>, btArgDefaultFor()

// ============================================================================
// BehaviorTreeRunner
// ============================================================================

class BehaviorTreeRunner {
public:
    struct StatusSnapshot {
        std::string treeState;     ///< IDLE / LOADED / RUNNING / SUCCESS / FAILURE / HALTED
        std::string currentNode;   ///< 当前正在执行的节点名
        std::string message;       ///< 当前节点的描述文本
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

    // -- 树生命周期 -------------------------------------------------------------

    /**
     * @brief 加载并编译 BehaviorTree XML。
     * @return true 成功，false 时 error 包含编译错误信息。
     * @note 加载前会先 stop() 当前正在运行的树。
     */
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

    /**
     * @brief 启动行为树 tick 循环。
     * @return true 成功，false 时 error 说明原因。
     * @note 若上次运行已完成但线程未 join，会先 join 再创建新线程。
     */
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

        if (tickThread_.joinable())
            tickThread_.join();
        running_.store(true, std::memory_order_release);
        tickThread_ = std::thread(&BehaviorTreeRunner::tickLoop, this);
        setStatus("RUNNING", "", "Tree started");
        return true;
    }

    /**
     * @brief 停止行为树执行。
     *
     * 设置停止标志 → 暂停树 → 等待 tick 线程退出 → join。
     * 可从任意线程调用，与 tickLoop 之间通过 mutex_ 同步。
     */
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

    /// 获取线程安全的当前状态快照。
    StatusSnapshot status() const
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        return status_;
    }

private:
    // -- 节点注册 ---------------------------------------------------------------

    /**
     * @brief 向 BT 工厂注册所有可用的命令节点。
     *
     * 包括 :
     *   - "SendCommand" : 通用 CSV args 节点（兼容旧 XML）
     *   - 每个 CmdId 对应的 TypedSendCommandNode<ArgEnum> 别名节点
     */
    void registerNodes()
    {
        // 通用 SendCommand 节点（旧 CSV 格式，兼容现有 XML）
        factory_.registerBuilder<zrcs_bt::SendCommandNode>(
            "SendCommand",
            [sharedState = sharedState_](const std::string& name, const BT::NodeConfiguration& config)
            {
                return std::make_unique<zrcs_bt::SendCommandNode>(name, config, sharedState);
            });

        // BatchCmd: 批量发送同类型命令，测试段间连续性
        factory_.registerBuilder<zrcs_bt::BatchCmdNode>(
            "BatchCmd",
            [sharedState = sharedState_](const std::string& name, const BT::NodeConfiguration& config)
            {
                return std::make_unique<zrcs_bt::BatchCmdNode>(name, config, sharedState);
            });

        // 为每个已实现的 CmdId 注册 TypedSendCommandNode 别名
        registerCommandAliasNodes();
    }

    /**
     * @brief 遍历 CmdId 枚举，为每个命令注册带命名输入端口的别名节点。
     *
     * switch 分派将运行时 CmdId 映射到编译期 ArgEnum 类型，
     * 然后调用 registerTypedAlias<ArgEnum>() 完成注册。
     */
    void registerCommandAliasNodes()
    {
        for (int value = Enable; value < SENTINEL; ++value)
        {
            const auto commandId = static_cast<CmdId>(value);
            const std::string commandName = zrcs::cmdIdToName(commandId);

            switch (commandId)
            {
            case CmdId::Enable:     registerTypedAlias<EnableArg>(commandName);     break;
            case CmdId::Disable:    registerTypedAlias<DisableArg>(commandName);    break;
            case CmdId::Reset:      registerTypedAlias<ResetArg>(commandName);      break;
            case CmdId::Setmode:    registerTypedAlias<SetmodeArg>(commandName);    break;
            case CmdId::SetZero:    registerTypedAlias<SetZeroArg>(commandName);    break;
            case CmdId::JogabsJ:    registerTypedAlias<JogabsJArg>(commandName);    break;
            case CmdId::JogJ:       registerTypedAlias<JogJArg>(commandName);       break;
            case CmdId::MoveAbs:    registerTypedAlias<MoveAbsArg>(commandName);    break;
            case CmdId::MoveAbsJ:   registerTypedAlias<MoveAbsJArg>(commandName);   break;
            case CmdId::MoveJ:      registerTypedAlias<MoveJArg>(commandName);      break;
            case CmdId::MoveL:      registerTypedAlias<MoveLArg>(commandName);      break;
            case CmdId::MoveC:      registerTypedAlias<MoveCArg>(commandName);      break;
            case CmdId::Movehome:   registerTypedAlias<void>(commandName);          break;
            case CmdId::MoveLGalvo: registerTypedAlias<MoveLGalvoArg>(commandName); break;
            default: break;
            }
        }
    }

    /// 为单个命令注册 TypedSendCommandNode<ArgEnum> 别名节点。
    template <typename ArgEnum>
    void registerTypedAlias(const std::string& commandName)
    {
        BT::TreeNodeManifest manifest{
            BT::NodeType::ACTION,
            commandName,
            zrcs_bt::TypedSendCommandNode<ArgEnum>::providedPorts(),
            "ZRCS RT command"
        };

        factory_.registerBuilder(
            manifest,
            [sharedState = sharedState_, commandName](
                const std::string& name, const BT::NodeConfiguration& config)
            {
                return std::make_unique<zrcs_bt::TypedSendCommandNode<ArgEnum>>(
                    name, config, sharedState, commandName);
            });
    }

    // -- tick 循环 ---------------------------------------------------------------

    /**
     * @brief 行为树主循环（运行在独立线程）。
     *
     * 每 20ms tick 一次树根节点，直到树返回 SUCCESS / FAILURE 或 running_ 被 stop() 置为 false。
     * 通过 sharedState_ 同步当前执行节点信息给 UI。
     */
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

    /// 线程安全地更新状态快照。
    void setStatus(const std::string& treeState,
                   const std::string& currentNode,
                   const std::string& message)
    {
        std::lock_guard<std::mutex> lock(statusMutex_);
        status_.treeState = treeState;
        status_.currentNode = currentNode;
        status_.message = message;
    }

    // -- 成员变量 ---------------------------------------------------------------

    RtBridge* bridge_;
    BT::BehaviorTreeFactory factory_;
    std::shared_ptr<zrcs_bt::SharedState> sharedState_;
    std::unique_ptr<BT::Tree> tree_;
    std::atomic<bool> running_{false};
    std::thread tickThread_;
    mutable std::mutex mutex_;        ///< 保护 tree_ 和 tick 同步
    mutable std::mutex statusMutex_;  ///< 保护 status_
    StatusSnapshot status_;
};

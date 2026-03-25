#pragma once

#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <spdlog/spdlog.h>
#include "rtBridge/rtBridge.h"

/**
 * @brief 行为树执行引擎，运行在 NRT 进程中
 * @details 接收 GUI 下发的行为树 XML，创建并 tick 执行行为树。
 *          Action 节点通过 RtBridge 向 RT 进程发送运动命令。
 */

// ============================================================================
// 全局指针：供 BT Action 节点访问 RtBridge
// ============================================================================
namespace BTGlobal {
    inline RtBridge* g_bridge = nullptr;
}

// ============================================================================
// 通用 BT Action 节点：通过 RtBridge 发送命令，等待 RT 完成
// ============================================================================

/**
 * @brief 通用运动命令节点，映射到 RT 进程的 CmdNode
 * @details 通过 BT Port "command" 指定命令名，"args" 指定参数
 *          onStart: 通过 RtBridge 发送命令
 *          onRunning: 轮询 RtBridge 判断命令是否完成
 */
class BTMotionAction : public BT::StatefulActionNode
{
public:
    BTMotionAction(const std::string& name, const BT::NodeConfiguration& config)
        : BT::StatefulActionNode(name, config), my_seq_(0)
    {}

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("command", "RT 命令名称，如 MoveJ/MoveL/Enable 等"),
            BT::InputPort<std::string>("args", "", "逗号分隔的参数列表，如 '1.0,2.0,3.0'"),
        };
    }

    BT::NodeStatus onStart() override
    {
        if (!BTGlobal::g_bridge) {
            spdlog::error("[BTMotionAction] RtBridge not available");
            return BT::NodeStatus::FAILURE;
        }

        std::string cmd_name;
        if (!getInput("command", cmd_name) || cmd_name.empty()) {
            spdlog::error("[BTMotionAction] Missing 'command' port");
            return BT::NodeStatus::FAILURE;
        }

        std::string args_str;
        getInput("args", args_str);

        auto [result, seq] = BTGlobal::g_bridge->sendCommand(cmd_name, args_str);
        if (result != RtBridge::SendResult::OK) {
            spdlog::error("[BTMotionAction] Failed to send '{}' (seq={})", cmd_name, seq);
            return BT::NodeStatus::FAILURE;
        }

        my_seq_ = seq;
        spdlog::info("[BTMotionAction] Sent command: '{}' args='{}' (seq={})", cmd_name, args_str, my_seq_);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        if (!BTGlobal::g_bridge) {
            return BT::NodeStatus::FAILURE;
        }

        if (!BTGlobal::g_bridge->isCommandCompleted(my_seq_)) {
            return BT::NodeStatus::RUNNING;
        }

        auto completion = BTGlobal::g_bridge->lastCompletion();
        if (completion.success) {
            spdlog::info("[BTMotionAction] Command seq={} completed: SUCCESS", my_seq_);
            return BT::NodeStatus::SUCCESS;
        } else {
            spdlog::error("[BTMotionAction] Command seq={} completed: FAILED", my_seq_);
            return BT::NodeStatus::FAILURE;
        }
    }

    void onHalted() override
    {
        spdlog::warn("[BTMotionAction] Action halted (seq={})", my_seq_);
    }

private:
    uint32_t my_seq_;
};

// ============================================================================
// 等待节点：暂停指定毫秒数
// ============================================================================
class BTWaitAction : public BT::StatefulActionNode
{
public:
    BTWaitAction(const std::string& name, const BT::NodeConfiguration& config)
        : BT::StatefulActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return { BT::InputPort<int>("ms", 1000, "等待时间（毫秒）") };
    }

    BT::NodeStatus onStart() override
    {
        int ms = 1000;
        getInput("ms", ms);
        deadline_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        if (std::chrono::steady_clock::now() >= deadline_) {
            return BT::NodeStatus::SUCCESS;
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override {}

private:
    std::chrono::steady_clock::time_point deadline_;
};

// ============================================================================
// BTEngine 主类
// ============================================================================
class BTEngine
{
public:
    enum class State { IDLE, RUNNING, SUCCESS, FAILURE, HALTED };

    explicit BTEngine(RtBridge* bridge)
        : bridge_(bridge), running_(false), state_(State::IDLE)
    {
        BTGlobal::g_bridge = bridge;
        registerNodes();
    }

    ~BTEngine()
    {
        stop();
    }

    /**
     * @brief 加载行为树 XML
     * @return 成功返回空字符串，失败返回错误信息
     */
    std::string loadTree(const std::string& xml)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // 如果正在运行，先停止
        if (running_) {
            stopInternal();
        }

        try {
            tree_ = factory_.createTreeFromText(xml);
            state_ = State::IDLE;
            current_node_name_.clear();
            spdlog::info("[BTEngine] Tree loaded successfully, xml_size={}", xml.size());
            return "";
        } catch (const std::exception& e) {
            std::string err = std::string("Failed to load tree: ") + e.what();
            spdlog::error("[BTEngine] {}", err);
            return err;
        }
    }

    /**
     * @brief 启动行为树执行
     */
    bool start()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (tree_.rootNode() == nullptr) {
            spdlog::error("[BTEngine] No tree loaded");
            return false;
        }
        if (running_) {
            spdlog::warn("[BTEngine] Already running");
            return false;
        }

        running_ = true;
        state_ = State::RUNNING;
        tick_thread_ = std::thread(&BTEngine::tickLoop, this);
        spdlog::info("[BTEngine] Execution started");
        return true;
    }

    /**
     * @brief 停止行为树执行
     */
    void stop()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopInternal();
    }

    /**
     * @brief 获取当前状态
     */
    State getState() const { return state_.load(); }

    std::string getStateString() const
    {
        switch (state_.load()) {
            case State::IDLE:    return "IDLE";
            case State::RUNNING: return "RUNNING";
            case State::SUCCESS: return "SUCCESS";
            case State::FAILURE: return "FAILURE";
            case State::HALTED:  return "HALTED";
        }
        return "UNKNOWN";
    }

    std::string getCurrentNodeName() const
    {
        std::lock_guard<std::mutex> lock(name_mutex_);
        return current_node_name_;
    }

private:
    void registerNodes()
    {
        // 通用运动命令节点 - 通过 "command" port 指定具体命令
        factory_.registerNodeType<BTMotionAction>("MotionAction");

        // 也注册与 RT 命令同名的便捷节点
        // 这些都使用同一个 BTMotionAction，但在 XML 中可以直接用命令名
        registerMotionAlias("MoveJ");
        registerMotionAlias("MoveL");
        registerMotionAlias("MoveC");
        registerMotionAlias("Enable");
        registerMotionAlias("Disable");
        registerMotionAlias("Reset");
        registerMotionAlias("JogJ");
        registerMotionAlias("JogabsJ");
        registerMotionAlias("ContinuousJog");
        registerMotionAlias("Home");
        registerMotionAlias("Stop");

        // 等待节点
        factory_.registerNodeType<BTWaitAction>("Wait");
    }

    /**
     * @brief 注册一个运动命令别名节点
     * @details 创建 SimpleAction，自动设置 command port 为节点名
     */
    void registerMotionAlias(const std::string& cmd_name)
    {
        BT::PortsList ports = {
            BT::InputPort<std::string>("args", "", "逗号分隔的参数列表"),
        };
        factory_.registerSimpleAction(cmd_name,
            [cmd_name](BT::TreeNode& self) -> BT::NodeStatus {
                if (!BTGlobal::g_bridge) {
                    spdlog::error("[BTAlias:{}] RtBridge not available", cmd_name);
                    return BT::NodeStatus::FAILURE;
                }

                std::string args_str;
                self.getInput("args", args_str);

                auto [result, seq] = BTGlobal::g_bridge->sendCommand(cmd_name, args_str);
                if (result != RtBridge::SendResult::OK) {
                    spdlog::error("[BTAlias:{}] Command queue full (seq={})", cmd_name, seq);
                    return BT::NodeStatus::FAILURE;
                }
                spdlog::info("[BTAlias:{}] Sent via RtBridge, args='{}', seq={}", cmd_name, args_str, seq);
                return BT::NodeStatus::SUCCESS;
            }, ports);
    }

    void tickLoop()
    {
        spdlog::info("[BTEngine] Tick loop started");
        while (running_) {
            BT::NodeStatus status = tree_.tickRoot();

            // 更新当前活跃节点名（用于状态上报）
            {
                std::lock_guard<std::mutex> lock(name_mutex_);
                current_node_name_.clear();
                for (auto& node : tree_.nodes) {
                    if (node->status() == BT::NodeStatus::RUNNING) {
                        current_node_name_ = node->name();
                        break;
                    }
                }
            }

            if (status == BT::NodeStatus::SUCCESS) {
                state_ = State::SUCCESS;
                running_ = false;
                spdlog::info("[BTEngine] Tree completed: SUCCESS");
                break;
            } else if (status == BT::NodeStatus::FAILURE) {
                state_ = State::FAILURE;
                running_ = false;
                spdlog::error("[BTEngine] Tree completed: FAILURE");
                break;
            }

            spdlog::trace("[BTEngine] Tick: RUNNING, active_node='{}'", current_node_name_);

            // RUNNING - 等待一段时间再 tick
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        spdlog::info("[BTEngine] Tick loop exited");
    }

    void stopInternal()
    {
        if (!running_) return;
        running_ = false;
        if (tick_thread_.joinable()) {
            tick_thread_.join();
        }
        if (tree_.rootNode()) {
            tree_.haltTree();
        }
        state_ = State::HALTED;
        spdlog::info("[BTEngine] Execution stopped");
    }

    RtBridge* bridge_;
    BT::BehaviorTreeFactory factory_;
    BT::Tree tree_;
    std::thread tick_thread_;
    std::atomic<bool> running_;
    std::atomic<State> state_;
    std::mutex mutex_;
    mutable std::mutex name_mutex_;
    std::string current_node_name_;
};

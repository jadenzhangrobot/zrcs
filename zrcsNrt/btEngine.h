#pragma once

#include <behaviortree_cpp_v3/bt_factory.h>
#include <behaviortree_cpp_v3/behavior_tree.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <cstring>
#include "sharedMemory/sharedData.h"

/**
 * @brief 行为树执行引擎，运行在 NRT 进程中
 * @details 接收 GUI 下发的行为树 XML，创建并 tick 执行行为树。
 *          Action 节点通过共享内存向 RT 进程发送运动命令。
 */

// ============================================================================
// 全局指针：供 BT Action 节点访问共享内存
// ============================================================================
namespace BTGlobal {
    inline SharedBlock* g_shared_block = nullptr;
    inline std::atomic<uint32_t> g_cmd_seq{1};  // 全局命令序列号递增器
}

// ============================================================================
// 通用 BT Action 节点：将命令推送到共享内存，等待 RT 完成
// ============================================================================

/**
 * @brief 通用运动命令节点，映射到 RT 进程的 CmdNode
 * @details 通过 BT Port "command" 指定命令名，"args" 指定参数
 *          onStart: 构造 Command 并 push 到共享内存队列
 *          onRunning: 轮询 lastCmdSeq 判断命令是否完成
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
        if (!BTGlobal::g_shared_block) {
            std::cerr << "[BTEngine] SharedBlock not available" << std::endl;
            return BT::NodeStatus::FAILURE;
        }

        // 获取命令名称
        std::string cmd_name;
        if (!getInput("command", cmd_name) || cmd_name.empty()) {
            std::cerr << "[BTEngine] Missing 'command' port" << std::endl;
            return BT::NodeStatus::FAILURE;
        }

        // 构造 Command
        Command shm_cmd{};
        strncpy(shm_cmd.cmd, cmd_name.c_str(), sizeof(shm_cmd.cmd) - 1);
        shm_cmd.cmd[sizeof(shm_cmd.cmd) - 1] = '\0';

        // 解析参数
        std::string args_str;
        if (getInput("args", args_str) && !args_str.empty()) {
            size_t idx = 0;
            std::string token;
            std::istringstream ss(args_str);
            while (std::getline(ss, token, ',') && idx < MAX_CMD_ARGS) {
                try {
                    shm_cmd.args[idx++] = std::stod(token);
                } catch (...) {
                    break;
                }
            }
        }

        // 分配序列号
        my_seq_ = BTGlobal::g_cmd_seq.fetch_add(1, std::memory_order_relaxed);
        shm_cmd.seq = my_seq_;

        // 推送到共享内存队列
        if (!BTGlobal::g_shared_block->commandQueue.push(shm_cmd)) {
            std::cerr << "[BTEngine] Command queue full, dropping: " << cmd_name << std::endl;
            return BT::NodeStatus::FAILURE;
        }

        std::cout << "[BTEngine] Sent command: " << cmd_name << " (seq=" << my_seq_ << ")" << std::endl;
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        if (!BTGlobal::g_shared_block) {
            return BT::NodeStatus::FAILURE;
        }

        uint32_t completed_seq = BTGlobal::g_shared_block->lastCmdSeq.load(std::memory_order_acquire);
        if (completed_seq == my_seq_) {
            uint8_t result = BTGlobal::g_shared_block->lastCmdResult.load(std::memory_order_acquire);
            if (result == 0) {
                return BT::NodeStatus::SUCCESS;
            } else {
                std::cerr << "[BTEngine] Command seq=" << my_seq_ << " failed" << std::endl;
                return BT::NodeStatus::FAILURE;
            }
        }
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override
    {
        std::cout << "[BTEngine] Action halted (seq=" << my_seq_ << ")" << std::endl;
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

    explicit BTEngine(SharedBlock* shared_block)
        : shared_block_(shared_block), running_(false), state_(State::IDLE)
    {
        BTGlobal::g_shared_block = shared_block;
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
            std::cout << "[BTEngine] Tree loaded successfully" << std::endl;
            return "";
        } catch (const std::exception& e) {
            std::string err = std::string("Failed to load tree: ") + e.what();
            std::cerr << "[BTEngine] " << err << std::endl;
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
            std::cerr << "[BTEngine] No tree loaded" << std::endl;
            return false;
        }
        if (running_) {
            std::cerr << "[BTEngine] Already running" << std::endl;
            return false;
        }

        running_ = true;
        state_ = State::RUNNING;
        tick_thread_ = std::thread(&BTEngine::tickLoop, this);
        std::cout << "[BTEngine] Execution started" << std::endl;
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
                // SimpleAction 是同步的，不适合等待 RT 完成
                // 所以这里只发送命令，不等待完成（fire-and-forget）
                if (!BTGlobal::g_shared_block) {
                    return BT::NodeStatus::FAILURE;
                }

                Command shm_cmd{};
                strncpy(shm_cmd.cmd, cmd_name.c_str(), sizeof(shm_cmd.cmd) - 1);

                std::string args_str;
                if (self.getInput("args", args_str) && !args_str.empty()) {
                    size_t idx = 0;
                    std::istringstream ss(args_str);
                    std::string token;
                    while (std::getline(ss, token, ',') && idx < MAX_CMD_ARGS) {
                        try { shm_cmd.args[idx++] = std::stod(token); } catch (...) { break; }
                    }
                }

                shm_cmd.seq = BTGlobal::g_cmd_seq.fetch_add(1, std::memory_order_relaxed);

                if (!BTGlobal::g_shared_block->commandQueue.push(shm_cmd)) {
                    return BT::NodeStatus::FAILURE;
                }
                return BT::NodeStatus::SUCCESS;
            }, ports);
    }

    void tickLoop()
    {
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
                std::cout << "[BTEngine] Tree completed: SUCCESS" << std::endl;
                break;
            } else if (status == BT::NodeStatus::FAILURE) {
                state_ = State::FAILURE;
                running_ = false;
                std::cout << "[BTEngine] Tree completed: FAILURE" << std::endl;
                break;
            }

            // RUNNING - 等待一段时间再 tick
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
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
        std::cout << "[BTEngine] Execution stopped" << std::endl;
    }

    SharedBlock* shared_block_;
    BT::BehaviorTreeFactory factory_;
    BT::Tree tree_;
    std::thread tick_thread_;
    std::atomic<bool> running_;
    std::atomic<State> state_;
    std::mutex mutex_;
    mutable std::mutex name_mutex_;
    std::string current_node_name_;
};

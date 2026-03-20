#pragma once
#include <zmq.hpp>
#include <thread>
#include <atomic>
#include <iostream>
#include <fstream>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
#include "message.pb.h"
#include "sharedMemory/sharedData.h"
#include "btEngine.h"

class ZMQServer {
private:
    zmq::context_t context_;
    std::unique_ptr<zmq::socket_t> socket_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    SharedBlock* shared_block_;
    BTEngine* bt_engine_;
    std::atomic<uint64_t> dropped_count_{0};

    static constexpr const char* ENDPOINT = "tcp://*:5555";
    static constexpr int RECV_TIMEOUT = 1000; // ms

public:
    ZMQServer(SharedBlock* shared_block, BTEngine* bt_engine)
        : context_(1), socket_(nullptr), running_(false),
          shared_block_(shared_block), bt_engine_(bt_engine) {}

    ~ZMQServer() {
        stop();
    }

    bool initialize() {
        try {
            socket_ = std::make_unique<zmq::socket_t>(context_, zmq::socket_type::rep);
            socket_->set(zmq::sockopt::rcvtimeo, RECV_TIMEOUT);
            socket_->set(zmq::sockopt::linger, 0);  // 关闭时不等待未发送消息
            socket_->bind(ENDPOINT);

            std::cout << "[ZMQServer] Initialized on " << ENDPOINT << std::endl;
            return true;
        } catch (const zmq::error_t& e) {
            std::cerr << "[ZMQServer] Initialization error: " << e.what() << std::endl;
            return false;
        }
    }

    void start() {
        if (running_.exchange(true)) {
            return; // Already running
        }
        server_thread_ = std::thread(&ZMQServer::run, this);
    }

    void stop() {
        if (!running_.exchange(false)) {
            return; // Already stopped
        }
        // 等待服务线程退出（recv 超时后会自然退出循环）
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
        // 显式关闭 socket 和 context，释放端口
        if (socket_) {
            socket_->close();
            socket_.reset();
        }
        context_.close();

        if (dropped_count_ > 0) {
            std::cout << "[ZMQServer] Total dropped commands: " << dropped_count_.load() << std::endl;
        }
        std::cout << "[ZMQServer] Stopped" << std::endl;
    }

    uint64_t droppedCount() const { return dropped_count_.load(); }

private:
    void run() {
        while (running_) {
            try {
                zmq::message_t request;
                auto result = socket_->recv(request, zmq::recv_flags::none);

                if (!result) {
                    continue;
                }

                // 先尝试解析为 TypedCommand（支持 BT 命令）
                zrcs_message::TypedCommand typed_cmd;
                if (typed_cmd.ParseFromArray(request.data(), request.size())
                    && typed_cmd.has_bt_command())
                {
                    handleBTCommand(typed_cmd.bt_command());
                    continue;
                }

                // 回退：解析为 MotionCommand（兼容现有协议）
                zrcs_message::MotionCommand cmd;
                if (!cmd.ParseFromArray(request.data(), request.size())) {
                    std::cerr << "[ZMQServer] Failed to parse protobuf message" << std::endl;
                    sendReply("ERROR: Parse failed");
                    continue;
                }

                // 转换为共享内存命令格式
                Command shm_cmd{};
                strncpy(shm_cmd.cmd, cmd.command().c_str(), sizeof(shm_cmd.cmd) - 1);
                shm_cmd.cmd[sizeof(shm_cmd.cmd) - 1] = '\0';

                // 复制参数（受 MAX_CMD_ARGS 限制）
                for (size_t i = 0; i < static_cast<size_t>(cmd.args_size()) && i < MAX_CMD_ARGS; ++i) {
                    shm_cmd.args[i] = cmd.args(i);
                }

                // 推送到共享内存队列
                if (shared_block_->commandQueue.push(shm_cmd)) {
                    std::cout << "[ZMQServer] Command received: " << cmd.command() << std::endl;
                    sendReply("OK");
                } else {
                    ++dropped_count_;
                    std::cerr << "[ZMQServer] Command queue full (dropped: "
                              << dropped_count_.load() << ")" << std::endl;
                    sendReply("ERROR: Queue full");
                }

            } catch (const zmq::error_t& e) {
                if (running_ && e.num() != EAGAIN) {
                    std::cerr << "[ZMQServer] Error: " << e.what() << std::endl;
                }
            }
        }
    }

    /**
     * @brief 获取可执行文件所在目录
     */
    static std::filesystem::path getExeDir() {
#ifdef _WIN32
        char buf[MAX_PATH];
        GetModuleFileNameA(nullptr, buf, MAX_PATH);
        return std::filesystem::path(buf).parent_path();
#else
        return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
    }

    /**
     * @brief 将 BT XML 保存到 config/bt/ 目录
     * @return 保存的文件路径，失败返回空字符串
     */
    std::string saveBTXml(const std::string& xml_data) {
        try {
            auto bt_dir = getExeDir() / "config" / "bt";
            std::filesystem::create_directories(bt_dir);

            // 用时间戳命名，同时维护一个 current.xml 始终指向最新
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            std::tm tm_buf{};
#ifdef _WIN32
            localtime_s(&tm_buf, &t);
#else
            localtime_r(&t, &tm_buf);
#endif
            char ts[64];
            std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm_buf);

            std::string filename = std::string("bt_") + ts + ".xml";
            auto filepath = bt_dir / filename;

            std::ofstream ofs(filepath, std::ios::out | std::ios::trunc);
            if (!ofs.is_open()) {
                std::cerr << "[ZMQServer] Failed to open file: " << filepath << std::endl;
                return "";
            }
            ofs << xml_data;
            ofs.close();

            // 覆盖写 current.xml，方便下次启动时加载最新树
            auto current_path = bt_dir / "current.xml";
            std::ofstream cur(current_path, std::ios::out | std::ios::trunc);
            if (cur.is_open()) {
                cur << xml_data;
                cur.close();
            }

            std::cout << "[ZMQServer] BT XML saved: " << filepath << std::endl;
            return filepath.string();
        } catch (const std::exception& e) {
            std::cerr << "[ZMQServer] Failed to save BT XML: " << e.what() << std::endl;
            return "";
        }
    }

    void handleBTCommand(const zrcs_message::BehaviorTreeCommand& bt_cmd) {
        const std::string& action = bt_cmd.action();

        if (action == "LOAD") {
            // 先保存到本地文件
            saveBTXml(bt_cmd.xml_data());

            // 再加载到行为树引擎
            std::string err = bt_engine_->loadTree(bt_cmd.xml_data());
            if (err.empty()) {
                sendReply("OK");
            } else {
                sendReply("ERROR: " + err);
            }
        } else if (action == "START") {
            if (bt_engine_->start()) {
                sendReply("OK");
            } else {
                sendReply("ERROR: Failed to start BT execution");
            }
        } else if (action == "STOP") {
            bt_engine_->stop();
            sendReply("OK");
        } else if (action == "STATUS") {
            // 返回 BehaviorTreeStatus protobuf 序列化
            zrcs_message::BehaviorTreeStatus status;
            status.set_tree_state(bt_engine_->getStateString());
            status.set_current_node(bt_engine_->getCurrentNodeName());
            std::string serialized;
            status.SerializeToString(&serialized);
            sendReplyRaw(serialized);
        } else {
            sendReply("ERROR: Unknown BT action: " + action);
        }
    }

    void sendReply(const std::string& message) {
        try {
            zmq::message_t reply(message.size());
            memcpy(reply.data(), message.data(), message.size());
            socket_->send(reply, zmq::send_flags::none);
        } catch (const zmq::error_t& e) {
            std::cerr << "[ZMQServer] Send reply error: " << e.what() << std::endl;
        }
    }

    void sendReplyRaw(const std::string& data) {
        try {
            zmq::message_t reply(data.size());
            memcpy(reply.data(), data.data(), data.size());
            socket_->send(reply, zmq::send_flags::none);
        } catch (const zmq::error_t& e) {
            std::cerr << "[ZMQServer] Send reply error: " << e.what() << std::endl;
        }
    }
};

/**
 * @file test_zmq_comm.cpp
 * @brief ZMQ 通信层测试
 * @details 测试上位机 ZMQ REQ 客户端 + Protobuf 序列化的完整通信链路
 *          内部启动 mock ZMQ REP 服务端线程，模拟下位机接收
 *
 * 测试项：
 *   1. MotionCommand 序列化 + 发送 + 服务端解析
 *   2. BehaviorTreeCommand (TypedCommand) 序列化 + 发送 + 服务端解析
 *   3. 多条命令连续发送
 *   4. 服务端超时（客户端无响应处理）
 */

#include <cassert>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <zmq.hpp>
#include "message.pb.h"

// 测试用端口，避免和正式服务冲突
static constexpr int TEST_PORT = 15555;
static constexpr const char* TEST_ENDPOINT = "tcp://127.0.0.1:15555";
static constexpr const char* TEST_BIND     = "tcp://*:15555";

// ============================================================================
// Mock 服务端：模拟 NRT ZMQServer
// ============================================================================

struct MockServerStats {
    std::atomic<int> motionCmdCount{0};
    std::atomic<int> btCmdCount{0};
    std::atomic<int> parseErrorCount{0};

    // 最后一条 MotionCommand 的内容
    std::string lastCmdName;
    std::vector<double> lastCmdArgs;

    // 最后一条 BT 命令
    std::string lastBTAction;
    std::string lastBTXml;
};

static void runMockServer(std::atomic<bool>& running, MockServerStats& stats)
{
    zmq::context_t ctx(1);
    zmq::socket_t socket(ctx, zmq::socket_type::rep);
    socket.set(zmq::sockopt::rcvtimeo, 500);
    socket.set(zmq::sockopt::linger, 0);
    socket.bind(TEST_BIND);

    while (running) {
        zmq::message_t request;
        auto result = socket.recv(request, zmq::recv_flags::none);
        if (!result) continue;

        // 尝试解析为 TypedCommand（BT 命令）
        zrcs_message::TypedCommand typed_cmd;
        if (typed_cmd.ParseFromArray(request.data(), request.size())
            && typed_cmd.has_bt_command())
        {
            stats.btCmdCount++;
            stats.lastBTAction = typed_cmd.bt_command().action();
            stats.lastBTXml = typed_cmd.bt_command().xml_data();

            // 回复 OK
            std::string reply = "OK";
            zmq::message_t rep(reply.size());
            memcpy(rep.data(), reply.data(), reply.size());
            socket.send(rep, zmq::send_flags::none);
            continue;
        }

        // 尝试解析为 MotionCommand
        zrcs_message::MotionCommand cmd;
        if (cmd.ParseFromArray(request.data(), request.size())) {
            stats.motionCmdCount++;
            stats.lastCmdName = cmd.command();
            stats.lastCmdArgs.clear();
            for (int i = 0; i < cmd.args_size(); ++i) {
                stats.lastCmdArgs.push_back(cmd.args(i));
            }

            std::string reply = "OK";
            zmq::message_t rep(reply.size());
            memcpy(rep.data(), reply.data(), reply.size());
            socket.send(rep, zmq::send_flags::none);
            continue;
        }

        // 解析失败
        stats.parseErrorCount++;
        std::string reply = "ERROR: Parse failed";
        zmq::message_t rep(reply.size());
        memcpy(rep.data(), reply.data(), reply.size());
        socket.send(rep, zmq::send_flags::none);
    }
}

// ============================================================================
// 辅助：客户端发送并接收回复
// ============================================================================

static std::string sendAndRecv(zmq::socket_t& socket, const std::string& data)
{
    zmq::message_t request(data.size());
    memcpy(request.data(), data.data(), data.size());
    socket.send(request, zmq::send_flags::none);

    zmq::message_t reply;
    auto result = socket.recv(reply, zmq::recv_flags::none);
    assert(result.has_value() && "Should receive a reply");
    return std::string(static_cast<char*>(reply.data()), reply.size());
}

// ============================================================================
// 测试 1：MotionCommand 序列化 + 发送 + 服务端解析
// ============================================================================

static void test_motion_command(zmq::socket_t& client, MockServerStats& stats)
{
    std::cout << "  [1] MotionCommand (MoveJ)..." << std::flush;

    zrcs_message::MotionCommand cmd;
    cmd.set_command("MoveJ");
    cmd.add_args(10.0);
    cmd.add_args(20.0);
    cmd.add_args(30.0);
    cmd.add_args(0.0);
    cmd.add_args(0.0);
    cmd.add_args(0.0);

    std::string serialized;
    assert(cmd.SerializeToString(&serialized));

    std::string reply = sendAndRecv(client, serialized);
    assert(reply == "OK");
    assert(stats.lastCmdName == "MoveJ");
    assert(stats.lastCmdArgs.size() == 6);
    assert(stats.lastCmdArgs[0] == 10.0);
    assert(stats.lastCmdArgs[1] == 20.0);
    assert(stats.lastCmdArgs[2] == 30.0);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// 测试 2：MotionCommand 无参数（Stop / Enable）
// ============================================================================

static void test_motion_command_no_args(zmq::socket_t& client, MockServerStats& stats)
{
    std::cout << "  [2] MotionCommand no args (Enable)..." << std::flush;

    zrcs_message::MotionCommand cmd;
    cmd.set_command("Enable");

    std::string serialized;
    assert(cmd.SerializeToString(&serialized));

    std::string reply = sendAndRecv(client, serialized);
    assert(reply == "OK");
    assert(stats.lastCmdName == "Enable");
    assert(stats.lastCmdArgs.empty());

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// 测试 3：BehaviorTreeCommand LOAD（TypedCommand 包装）
// ============================================================================

static void test_bt_load(zmq::socket_t& client, MockServerStats& stats)
{
    std::cout << "  [3] BT LOAD command..." << std::flush;

    zrcs_message::TypedCommand typed_cmd;
    auto* bt_cmd = typed_cmd.mutable_bt_command();
    bt_cmd->set_action("LOAD");
    bt_cmd->set_xml_data("<root><Sequence><MoveJ args=\"1,2,3\"/></Sequence></root>");

    std::string serialized;
    assert(typed_cmd.SerializeToString(&serialized));

    std::string reply = sendAndRecv(client, serialized);
    assert(reply == "OK");
    assert(stats.lastBTAction == "LOAD");
    assert(stats.lastBTXml.find("<root>") != std::string::npos);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// 测试 4：BT START / STOP（无 XML）
// ============================================================================

static void test_bt_start_stop(zmq::socket_t& client, MockServerStats& stats)
{
    std::cout << "  [4] BT START/STOP commands..." << std::flush;

    // START
    {
        zrcs_message::TypedCommand typed_cmd;
        auto* bt_cmd = typed_cmd.mutable_bt_command();
        bt_cmd->set_action("START");

        std::string serialized;
        assert(typed_cmd.SerializeToString(&serialized));

        std::string reply = sendAndRecv(client, serialized);
        assert(reply == "OK");
        assert(stats.lastBTAction == "START");
        assert(stats.lastBTXml.empty());
    }

    // STOP
    {
        zrcs_message::TypedCommand typed_cmd;
        auto* bt_cmd = typed_cmd.mutable_bt_command();
        bt_cmd->set_action("STOP");

        std::string serialized;
        assert(typed_cmd.SerializeToString(&serialized));

        std::string reply = sendAndRecv(client, serialized);
        assert(reply == "OK");
        assert(stats.lastBTAction == "STOP");
    }

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// 测试 5：连续发送多条命令
// ============================================================================

static void test_burst_commands(zmq::socket_t& client, MockServerStats& stats)
{
    std::cout << "  [5] Burst 20 commands..." << std::flush;

    int startCount = stats.motionCmdCount.load();

    for (int i = 0; i < 20; ++i) {
        zrcs_message::MotionCommand cmd;
        cmd.set_command("JogJ");
        cmd.add_args(static_cast<double>(i));      // axis
        cmd.add_args(static_cast<double>(i * 10));  // position

        std::string serialized;
        assert(cmd.SerializeToString(&serialized));

        std::string reply = sendAndRecv(client, serialized);
        assert(reply == "OK");
    }

    assert(stats.motionCmdCount.load() - startCount == 20);
    assert(stats.lastCmdName == "JogJ");
    assert(stats.lastCmdArgs.size() == 2);
    assert(stats.lastCmdArgs[0] == 19.0);
    assert(stats.lastCmdArgs[1] == 190.0);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// 测试 6：客户端超时（服务端不回复）
// ============================================================================

static void test_client_timeout()
{
    std::cout << "  [6] Client timeout (no server)..." << std::flush;

    // 连接到一个没有服务端的端口
    zmq::context_t ctx(1);
    zmq::socket_t socket(ctx, zmq::socket_type::req);
    socket.set(zmq::sockopt::rcvtimeo, 500); // 500ms 超时
    socket.set(zmq::sockopt::linger, 0);
    socket.connect("tcp://127.0.0.1:15556"); // 没有服务端

    zrcs_message::MotionCommand cmd;
    cmd.set_command("Test");
    std::string serialized;
    cmd.SerializeToString(&serialized);

    zmq::message_t request(serialized.size());
    memcpy(request.data(), serialized.data(), serialized.size());
    socket.send(request, zmq::send_flags::none);

    zmq::message_t reply;
    auto result = socket.recv(reply, zmq::recv_flags::none);
    assert(!result.has_value() && "Should timeout with no server");

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// 测试 7：统计验证
// ============================================================================

static void test_stats_summary(MockServerStats& stats)
{
    std::cout << "  [7] Stats summary..." << std::flush;

    // test1: 1 MoveJ + test2: 1 Enable + test5: 20 JogJ = 22
    assert(stats.motionCmdCount.load() == 22);
    // test3: 1 LOAD + test4: 1 START + 1 STOP = 3
    assert(stats.btCmdCount.load() == 3);
    assert(stats.parseErrorCount.load() == 0);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// main
// ============================================================================

int main()
{
    std::cout << "=== ZMQ Communication Test ===" << std::endl;

    // 启动 mock 服务端
    std::atomic<bool> serverRunning{true};
    MockServerStats stats;
    std::thread serverThread(runMockServer, std::ref(serverRunning), std::ref(stats));

    // 等服务端 bind 完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 创建客户端
    zmq::context_t ctx(1);
    zmq::socket_t client(ctx, zmq::socket_type::req);
    client.set(zmq::sockopt::rcvtimeo, 5000);
    client.set(zmq::sockopt::linger, 0);
    client.connect(TEST_ENDPOINT);

    // 运行测试
    test_motion_command(client, stats);
    test_motion_command_no_args(client, stats);
    test_bt_load(client, stats);
    test_bt_start_stop(client, stats);
    test_burst_commands(client, stats);
    test_client_timeout();
    test_stats_summary(stats);

    // 清理
    client.close();
    ctx.close();
    serverRunning = false;
    serverThread.join();

    std::cout << "\n=== All 7 tests PASSED ===" << std::endl;
    return 0;
}

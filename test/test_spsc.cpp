/**
 * @file test_spsc.cpp
 * @brief SPSCRingBuffer 单元测试
 * @details 测试基本读写、满/空边界、并发安全性
 */

#include <cassert>
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include "shared_memory/ShmLayout.h"

// ===== 基础功能测试 =====

static void test_push_pop_basic() {
    SPSCRingBuffer<int, 8> buf;

    // 空缓冲区 pop 应失败
    int val = -1;
    assert(!buf.pop(val));

    // push 然后 pop
    assert(buf.push(42));
    assert(buf.pop(val));
    assert(val == 42);

    // pop 后再 pop 应失败
    assert(!buf.pop(val));

    std::cout << "[PASS] test_push_pop_basic" << std::endl;
}

static void test_fill_and_overflow() {
    SPSCRingBuffer<int, 8> buf;

    // 容量为 8 时，可用槽位为 7（SPSC 浪费一个槽位判满）
    for (int i = 0; i < 7; ++i) {
        assert(buf.push(i));
    }
    // 第 8 个应失败（缓冲区满）
    assert(!buf.push(99));

    // 依次弹出验证顺序
    for (int i = 0; i < 7; ++i) {
        int val = -1;
        assert(buf.pop(val));
        assert(val == i);
    }
    // 全部弹出后应为空
    assert(!buf.pop(*reinterpret_cast<int*>(&buf)));

    std::cout << "[PASS] test_fill_and_overflow" << std::endl;
}

static void test_wrap_around() {
    SPSCRingBuffer<int, 8> buf;

    // 写 5 个、读 5 个，再写 5 个、读 5 个 —— 迫使 head/tail 回绕
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < 5; ++i) {
            assert(buf.push(round * 10 + i));
        }
        for (int i = 0; i < 5; ++i) {
            int val = -1;
            assert(buf.pop(val));
            assert(val == round * 10 + i);
        }
    }

    std::cout << "[PASS] test_wrap_around" << std::endl;
}

// ===== 并发测试 =====

static void test_concurrent_producer_consumer() {
    constexpr int N = 100000;
    SPSCRingBuffer<int, 64> buf;

    std::atomic<bool> done{false};
    std::vector<int> received;
    received.reserve(N);

    // 消费者线程
    std::thread consumer([&]() {
        int val;
        int count = 0;
        while (count < N) {
            if (buf.pop(val)) {
                received.push_back(val);
                ++count;
            }
        }
        done = true;
    });

    // 生产者（当前线程）
    for (int i = 0; i < N; ++i) {
        while (!buf.push(i)) {
            // 自旋等待消费者腾出空间
        }
    }

    consumer.join();

    // 验证所有数据按序到达
    assert(static_cast<int>(received.size()) == N);
    for (int i = 0; i < N; ++i) {
        assert(received[i] == i);
    }

    std::cout << "[PASS] test_concurrent_producer_consumer (" << N << " items)" << std::endl;
}

// ===== size() 监控测试 =====

static void test_size() {
    SPSCRingBuffer<int, 16> buf;
    assert(buf.size() == 0);

    buf.push(1);
    buf.push(2);
    buf.push(3);
    assert(buf.size() == 3);

    int val;
    buf.pop(val);
    assert(buf.size() == 2);

    std::cout << "[PASS] test_size" << std::endl;
}

// ===== Command 结构体在 SPSC 中的使用 =====

static void test_command_in_buffer() {
    SPSCRingBuffer<Command, 64> buf;

    Command cmd{};
    strncpy(cmd.cmd, "MoveJ", sizeof(cmd.cmd) - 1);
    cmd.args[0] = 1.0;
    cmd.args[1] = 2.0;
    cmd.args[MAX_CMD_ARGS - 1] = 99.9;

    assert(buf.push(cmd));

    Command out{};
    assert(buf.pop(out));
    assert(std::string(out.cmd) == "MoveJ");
    assert(out.args[0] == 1.0);
    assert(out.args[1] == 2.0);
    assert(out.args[MAX_CMD_ARGS - 1] == 99.9);

    std::cout << "[PASS] test_command_in_buffer" << std::endl;
}

int main() {
    std::cout << "===== SPSCRingBuffer Unit Tests =====" << std::endl;

    test_push_pop_basic();
    test_fill_and_overflow();
    test_wrap_around();
    test_size();
    test_command_in_buffer();
    test_concurrent_producer_consumer();

    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}

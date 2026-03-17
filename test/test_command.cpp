/**
 * @file test_command.cpp
 * @brief Command 结构体安全性测试
 * @details 测试缓冲区边界、零初始化、strncpy 安全使用
 */

#include <cassert>
#include <iostream>
#include <cstring>
#include "sharedMemory/sharedData.h"

static void test_command_zero_init() {
    Command cmd{};

    // 验证零初始化
    assert(cmd.cmd[0] == '\0');
    for (size_t i = 0; i < MAX_CMD_ARGS; ++i) {
        assert(cmd.args[i] == 0.0);
    }

    std::cout << "[PASS] test_command_zero_init" << std::endl;
}

static void test_command_name_truncation() {
    Command cmd{};

    // 构造一个超长字符串（>100 字节）
    std::string longName(200, 'X');
    strncpy(cmd.cmd, longName.c_str(), sizeof(cmd.cmd) - 1);
    cmd.cmd[sizeof(cmd.cmd) - 1] = '\0';

    // 验证被截断到 MAX_CMD_NAME - 1
    assert(strlen(cmd.cmd) == MAX_CMD_NAME - 1);
    // 验证以空字符结尾
    assert(cmd.cmd[MAX_CMD_NAME - 1] == '\0');

    std::cout << "[PASS] test_command_name_truncation" << std::endl;
}

static void test_command_max_args() {
    Command cmd{};

    // 填满所有参数槽位
    for (size_t i = 0; i < MAX_CMD_ARGS; ++i) {
        cmd.args[i] = static_cast<double>(i) * 1.5;
    }

    // 验证读回
    for (size_t i = 0; i < MAX_CMD_ARGS; ++i) {
        assert(cmd.args[i] == static_cast<double>(i) * 1.5);
    }

    std::cout << "[PASS] test_command_max_args (MAX_CMD_ARGS=" << MAX_CMD_ARGS << ")" << std::endl;
}

static void test_constants() {
    // 验证常量值符合预期
    assert(MAX_CMD_NAME == 100);
    assert(MAX_CMD_ARGS == 20);
    assert(COMMAND_BUFFER_SIZE == 64);
    assert(STATUS_BUFFER_SIZE == 64);

    // 验证 COMMAND_BUFFER_SIZE 是 2 的幂（SPSC 要求）
    assert((COMMAND_BUFFER_SIZE & (COMMAND_BUFFER_SIZE - 1)) == 0);
    assert((STATUS_BUFFER_SIZE & (STATUS_BUFFER_SIZE - 1)) == 0);

    std::cout << "[PASS] test_constants" << std::endl;
}

int main() {
    std::cout << "===== Command Safety Tests =====" << std::endl;

    test_command_zero_init();
    test_command_name_truncation();
    test_command_max_args();
    test_constants();

    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}

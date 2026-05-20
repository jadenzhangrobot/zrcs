#include "shared_memory/ShmLayout.h"

#include <cassert>
#include <iostream>

namespace {

void test_command_zero_init()
{
    zrcs::Command cmd{};
    assert(cmd.seq == 0);
    assert(cmd.cmdId == 0);
    for (size_t i = 0; i < zrcs::kCmdArgsMax; ++i) {
        assert(cmd.args[i] == 0.0);
    }
}

void test_command_values()
{
    zrcs::Command cmd{};
    cmd.seq = 123;
    cmd.cmdId = 17;
    for (size_t i = 0; i < zrcs::kCmdArgsMax; ++i) {
        cmd.args[i] = static_cast<double>(i) * 1.5;
    }

    assert(cmd.seq == 123);
    assert(cmd.cmdId == 17);
    for (size_t i = 0; i < zrcs::kCmdArgsMax; ++i) {
        assert(cmd.args[i] == static_cast<double>(i) * 1.5);
    }
}

void test_constants()
{
    assert(zrcs::kCmdArgsMax == 24);
    assert((zrcs::kCmdQueueCap & (zrcs::kCmdQueueCap - 1)) == 0);
    assert((zrcs::kLogQueueCap & (zrcs::kLogQueueCap - 1)) == 0);
}

} // namespace

int main()
{
    test_command_zero_init();
    test_command_values();
    test_constants();
    std::cout << "All Command tests passed." << std::endl;
    return 0;
}

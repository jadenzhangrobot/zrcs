#include "shared_memory/ShmLayout.h"

#include <cassert>
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

namespace {

template <typename T, size_t Cap>
struct QueueHarness {
    zrcs::ShmSPSC<T, Cap> queue;
    zrcs::ShmSPSCProducer<T, Cap> producer{queue};
    zrcs::ShmSPSCConsumer<T, Cap> consumer{queue};
};

void test_push_pop_basic()
{
    QueueHarness<int, 8> h;
    int val = -1;
    assert(!h.consumer.pop(val));
    assert(h.producer.push(42));
    assert(h.consumer.pop(val));
    assert(val == 42);
    assert(!h.consumer.pop(val));
}

void test_fill_and_overflow()
{
    QueueHarness<int, 8> h;
    for (int i = 0; i < 7; ++i) {
        assert(h.producer.push(i));
    }
    assert(!h.producer.push(99));
    for (int i = 0; i < 7; ++i) {
        int val = -1;
        assert(h.consumer.pop(val));
        assert(val == i);
    }
    int val = -1;
    assert(!h.consumer.pop(val));
}

void test_wrap_around()
{
    QueueHarness<int, 8> h;
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < 5; ++i) {
            assert(h.producer.push(round * 10 + i));
        }
        for (int i = 0; i < 5; ++i) {
            int val = -1;
            assert(h.consumer.pop(val));
            assert(val == round * 10 + i);
        }
    }
}

void test_concurrent_producer_consumer()
{
    constexpr int N = 100000;
    QueueHarness<int, 64> h;
    std::vector<int> received;
    received.reserve(N);

    std::thread consumer([&]() {
        int val = 0;
        while (static_cast<int>(received.size()) < N) {
            if (h.consumer.pop(val)) {
                received.push_back(val);
            }
        }
    });

    for (int i = 0; i < N; ++i) {
        while (!h.producer.push(i)) {
        }
    }
    consumer.join();

    assert(static_cast<int>(received.size()) == N);
    for (int i = 0; i < N; ++i) {
        assert(received[i] == i);
    }
}

void test_command_in_buffer()
{
    QueueHarness<zrcs::Command, 64> h;
    zrcs::Command cmd{};
    cmd.cmdId = 7;
    cmd.seq = 42;
    cmd.args[0] = 1.0;
    cmd.args[1] = 2.0;
    cmd.args[zrcs::kCmdArgsMax - 1] = 99.9;

    assert(h.producer.push(cmd));

    zrcs::Command out{};
    assert(h.consumer.pop(out));
    assert(out.cmdId == 7);
    assert(out.seq == 42);
    assert(out.args[0] == 1.0);
    assert(out.args[1] == 2.0);
    assert(out.args[zrcs::kCmdArgsMax - 1] == 99.9);
}

} // namespace

int main()
{
    test_push_pop_basic();
    test_fill_and_overflow();
    test_wrap_around();
    test_command_in_buffer();
    test_concurrent_producer_consumer();
    std::cout << "All SPSC tests passed." << std::endl;
    return 0;
}

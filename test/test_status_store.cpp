#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "status/StatusStore.h"

namespace {

zrcs::AxisFeedbackData makeFeedback(uint64_t sequence, uint64_t timeNs, double base)
{
    zrcs::AxisFeedbackData feedback{};
    feedback.sequence = sequence;
    feedback.simulationTimeNs = timeNs;
    feedback.axisCount = 2;
    for (size_t axis = 0; axis < feedback.axisCount; ++axis) {
        const double value = base + static_cast<double>(axis);
        feedback.position[axis] = value;
        feedback.cmdPosition[axis] = value + 0.1;
        feedback.cmdVelocity[axis] = value + 0.2;
        feedback.velocity[axis] = value + 0.3;
        feedback.torque[axis] = value + 0.4;
    }
    return feedback;
}

void testBatchIsPreservedAndCurrentValueIsReReadable()
{
    StatusStore store;

    // 批量写入 3 帧。
    std::vector<zrcs::AxisFeedbackData> batch;
    batch.push_back(makeFeedback(1, 1'000'000, 10.0));
    batch.push_back(makeFeedback(2, 2'000'000, 20.0));
    batch.push_back(makeFeedback(3, 3'000'000, 30.0));
    store.updateAxisFeedbackBatch(batch);

    // 写入一份当前值，验证 re-readable 语义。
    store.updateSystemMeta(42, 0, "RUN");
    store.setBtStatus({"RUNNING", "MoveTo", "ok"});

    const auto first = store.snapshot();

    // 反馈帧逐帧入队、逐帧搬出。
    assert(first.axisFeedbackFrames.size() == 3);
    for (size_t i = 0; i < first.axisFeedbackFrames.size(); ++i) {
        const auto& frame = first.axisFeedbackFrames[i];
        assert(frame.sequence == i + 1);
        assert(frame.simulationTimeNs == (i + 1) * 1'000'000);
        assert(frame.axes.size() == 2);
        assert(frame.axes[0].position == 10.0 * static_cast<double>(i + 1));
    }

    // 最后帧（序列号最大）的轴位置应反映 batch.back() 的值。
    const auto& lastFrame = first.axisFeedbackFrames.back();
    assert(lastFrame.axes[0].position == 30.0);
    assert(lastFrame.axes[1].position == 31.0);

    // —— 当前值部分：可重复读到 ——
    assert(first.heartbeat == 42);
    assert(first.systemState == "RUN");
    assert(first.bt.treeState == "RUNNING");
    assert(first.bt.currentNode == "MoveTo");

    const auto second = store.snapshot();
    // 队列部分已搬空。
    assert(second.axisFeedbackFrames.empty());
    // 当前值部分仍可读到。
    assert(second.heartbeat == 42);
    assert(second.systemState == "RUN");
    assert(second.bt.treeState == "RUNNING");
}

} // namespace

int main()
{
    testBatchIsPreservedAndCurrentValueIsReReadable();
    std::cout << "StatusStore feedback batch test PASSED\n";
    return 0;
}

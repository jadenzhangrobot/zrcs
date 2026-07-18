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

void testBatchIsPreservedAndLatestIsUpdated()
{
    StatusStore store;
    std::vector<zrcs::AxisFeedbackData> batch;
    batch.push_back(makeFeedback(1, 1'000'000, 10.0));
    batch.push_back(makeFeedback(2, 2'000'000, 20.0));
    batch.push_back(makeFeedback(3, 3'000'000, 30.0));

    store.updateAxisFeedbackBatch(batch);
    const auto first = store.snapshot();

    assert(first.hasAxisFeedback);
    assert(first.axes.size() == 2);
    assert(first.axes[0].position == 30.0);
    assert(first.axes[1].position == 31.0);

    assert(first.axisFeedbackFrames.size() == 3);
    for (size_t i = 0; i < first.axisFeedbackFrames.size(); ++i) {
        const auto& frame = first.axisFeedbackFrames[i];
        assert(frame.sequence == i + 1);
        assert(frame.simulationTimeNs == (i + 1) * 1'000'000);
        assert(frame.axes.size() == 2);
        assert(frame.axes[0].position == 10.0 * static_cast<double>(i + 1));
    }

    const auto second = store.snapshot();
    assert(second.hasAxisFeedback);
    assert(second.axes.size() == 2);
    assert(second.axes[0].position == 30.0);
    assert(second.axisFeedbackFrames.empty());
}

} // namespace

int main()
{
    testBatchIsPreservedAndLatestIsUpdated();
    std::cout << "StatusStore feedback batch test PASSED\n";
    return 0;
}

#pragma once

#include "system/node/BaseNodeInterface.h"
#include "system/node/NodeFactory.h"

#include <array>
#include <cstdint>
#include <vector>

class MoveExcite : public zrcsSystem::CmdNode {
public:
    MoveExcite();

    bool init() override;
    zrcsSystem::RunResult run() override;
    bool exit() override;

private:
    struct WaveState {
        double wave = 0.0;
        double dWave = 0.0;
        double ddWave = 0.0;
    };

    struct AxisState {
        int axisId = -1;
        double startPos = 0.0;
        double centerOffset = 0.0;
        double lastCommandPos = 0.0;
        double lastCommandVel = 0.0;
        double lastCommandAcc = 0.0;
        double maxVel = 0.0;
        double maxAcc = 0.0;
        double phaseOffset = 0.0;
    };

    static constexpr int kTermCount = 5;

    WaveState evaluateWave(double t, double phaseOffset) const;
    void publishCommand(bool valid);

    int requestedAxisId_ = -1;
    double durationSec_ = 20.0;
    double amplitude_ = 0.0;
    double centerOffset_ = 0.0;
    double rampTimeSec_ = 1.0;
    double dtSec_ = 0.001;
    uint32_t sessionId_ = 0;
    uint64_t tick_ = 0;
    uint64_t commandSeq_ = 0;
    std::array<double, kTermCount> frequencies_{};
    std::array<double, kTermCount> phases_{};
    std::vector<AxisState> axes_;
};

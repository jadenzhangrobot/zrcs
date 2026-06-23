#include "command/MoveExcite.h"

#include "config/Parameter.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace {

constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kLimitTolerance = 1e-5;

double argOrDefault(const zrcs::Command* cmd, MoveExciteArg arg, double fallback)
{
    const auto idx = static_cast<size_t>(arg);
    if (!cmd || idx >= zrcs::kCmdArgsMax) {
        return fallback;
    }
    const double value = cmd->args[idx];
    return value == 0.0 ? fallback : value;
}

double smoothstep(double x)
{
    x = std::clamp(x, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

double smoothstepDot01(double x)
{
    x = std::clamp(x, 0.0, 1.0);
    return 6.0 * x - 6.0 * x * x;
}

double smoothstepDDot01(double x)
{
    x = std::clamp(x, 0.0, 1.0);
    return 6.0 - 12.0 * x;
}

bool clampNearLimit(double& q, double lower, double upper)
{
    if (q < lower) {
        if (lower - q > kLimitTolerance) {
            return false;
        }
        q = lower;
    } else if (q > upper) {
        if (q - upper > kLimitTolerance) {
            return false;
        }
        q = upper;
    }
    return true;
}

} // namespace

MoveExcite::MoveExcite()
{
    std::strcpy(nodeName_, "MoveExcite");
    frequencies_ = {0.15, 0.35, 0.70, 1.10, 1.70};
    phases_ = {0.0, 1.3, 2.1, 0.7, 2.8};
}

bool MoveExcite::init()
{
    requestedAxisId_ =
        static_cast<int>(command_->args[static_cast<size_t>(MoveExciteArg::AxisId)]);
    durationSec_ = argOrDefault(command_, MoveExciteArg::Duration, 20.0);
    amplitude_ = command_->args[static_cast<size_t>(MoveExciteArg::Amplitude)];
    centerOffset_ = command_->args[static_cast<size_t>(MoveExciteArg::CenterOffset)];
    rampTimeSec_ = argOrDefault(command_, MoveExciteArg::RampTime, 1.0);
    sessionId_ = static_cast<uint32_t>(
        command_->args[static_cast<size_t>(MoveExciteArg::SessionId)]);
    commandSeq_ = command_ ? command_->seq : 0;

    const std::array<MoveExciteArg, kTermCount> freqArgs = {
        MoveExciteArg::F1, MoveExciteArg::F2, MoveExciteArg::F3,
        MoveExciteArg::F4, MoveExciteArg::F5};
    const std::array<MoveExciteArg, kTermCount> phaseArgs = {
        MoveExciteArg::Phase1, MoveExciteArg::Phase2, MoveExciteArg::Phase3,
        MoveExciteArg::Phase4, MoveExciteArg::Phase5};

    for (int i = 0; i < kTermCount; ++i) {
        frequencies_[i] = argOrDefault(command_, freqArgs[i], frequencies_[i]);
        phases_[i] = argOrDefault(command_, phaseArgs[i], phases_[i]);
        if (frequencies_[i] <= 0.0 || !std::isfinite(frequencies_[i])) {
            ERROR_PRINT("MoveExcite: frequency %d is invalid\n", i + 1);
            return false;
        }
    }

    if (durationSec_ <= 0.0 || !std::isfinite(durationSec_)) {
        ERROR_PRINT("MoveExcite: duration is invalid\n");
        return false;
    }
    if (amplitude_ <= 0.0 || !std::isfinite(amplitude_)) {
        ERROR_PRINT("MoveExcite: amplitude must be positive\n");
        return false;
    }
    if (rampTimeSec_ < 0.0 || !std::isfinite(rampTimeSec_)) {
        ERROR_PRINT("MoveExcite: rampTime is invalid\n");
        return false;
    }

    dtSec_ = cycletime * 0.001;
    tick_ = 0;

    const double velScale = argOrDefault(command_, MoveExciteArg::VelScale, 0.5);
    const double accScale = argOrDefault(command_, MoveExciteArg::AccScale, 0.5);

    const int axisCount = static_cast<int>(controller_->axes_.size());
    std::vector<int> axisIds;
    if (requestedAxisId_ >= 0) {
        if (requestedAxisId_ >= axisCount) {
            ERROR_PRINT("MoveExcite: axis %d out of range\n", requestedAxisId_);
            return false;
        }
        axisIds.push_back(requestedAxisId_);
    } else {
        int exciteCount = -requestedAxisId_;
        if (exciteCount <= 0 || exciteCount > axisCount) {
            exciteCount = axisCount;
        }
        axisIds.reserve(static_cast<size_t>(exciteCount));
        for (int axisId = 0; axisId < exciteCount; ++axisId) {
            axisIds.push_back(axisId);
        }
    }

    if (axisIds.empty()) {
        ERROR_PRINT("MoveExcite: no axis selected\n");
        return false;
    }

    constexpr double phaseStride = 0.73;
    const double requestedAmplitude = amplitude_;
    bool amplitudeAdjusted = false;

    for (int attempt = 0; attempt < 8; ++attempt) {
        double waveWorstVel = 0.0;
        double waveWorstAcc = 0.0;
        for (int i = 0; i < kTermCount; ++i) {
            const double w = 1.0 / static_cast<double>(kTermCount);
            const double omega = 2.0 * kPi * frequencies_[i];
            waveWorstVel += amplitude_ * w * omega;
            waveWorstAcc += amplitude_ * w * omega * omega;
        }

        bool retry = false;
        axes_.clear();
        axes_.reserve(axisIds.size());

        for (size_t i = 0; i < axisIds.size(); ++i) {
            const int axisId = axisIds[i];
            auto* axis = controller_->axes_[axisId].get();
            AxisState state{};
            const double lower = axis->getNegativeLimit();
            const double upper = axis->getPositiveLimit();
            double startPos = axis->actualPos();
            if (!clampNearLimit(startPos, lower, upper)) {
                ERROR_PRINT("MoveExcite: axis %d start position %.6f exceeds [%.6f, %.6f]\n",
                            axisId, startPos, lower, upper);
                return false;
            }

            state.axisId = axisId;
            state.startPos = startPos;
            state.centerOffset = centerOffset_;
            state.lastCommandPos = state.startPos;
            state.maxVel = axis->getMaxVelocity() * std::clamp(velScale, 0.05, 1.0);
            state.maxAcc = axis->getMaxAcceleration() * std::clamp(accScale, 0.05, 1.0);
            state.phaseOffset = static_cast<double>(i) * phaseStride;

            const double minCenter = lower + amplitude_ - state.startPos;
            const double maxCenter = upper - amplitude_ - state.startPos;
            if (minCenter > maxCenter) {
                const double rangeLimitedAmplitude = std::max(0.0, (upper - lower) * 0.5);
                if (rangeLimitedAmplitude <= 1e-6 || rangeLimitedAmplitude >= amplitude_) {
                    ERROR_PRINT("MoveExcite: axis %d limit range is too small for amplitude %.6f\n",
                                axisId, amplitude_);
                    return false;
                }
                amplitude_ = rangeLimitedAmplitude * 0.95;
                amplitudeAdjusted = true;
                retry = true;
                break;
            }
            state.centerOffset = std::clamp(state.centerOffset, minCenter, maxCenter);

            double worstVel = waveWorstVel;
            double worstAcc = waveWorstAcc;
            if (rampTimeSec_ > 0.0) {
                worstVel += std::abs(state.centerOffset) * 1.5 / rampTimeSec_;
                worstAcc += std::abs(state.centerOffset) * 6.0 /
                            (rampTimeSec_ * rampTimeSec_);
            }

            double scale = 1.0;
            if (worstVel > state.maxVel && worstVel > 0.0) {
                scale = std::min(scale, state.maxVel / worstVel);
            }
            if (worstAcc > state.maxAcc && worstAcc > 0.0) {
                scale = std::min(scale, state.maxAcc / worstAcc);
            }
            if (scale < 1.0) {
                const double newAmplitude = amplitude_ * std::clamp(scale * 0.95, 0.05, 0.95);
                if (newAmplitude < 1e-6 || newAmplitude >= amplitude_) {
                    ERROR_PRINT("MoveExcite: axis %d excitation exceeds velocity/acceleration limits\n",
                                axisId);
                    return false;
                }
                amplitude_ = newAmplitude;
                amplitudeAdjusted = true;
                retry = true;
                break;
            }

            axis->setAxisPositionCmd(state.startPos);
            axis->syncCmdHistory();
            axes_.push_back(state);
        }

        if (!retry) {
            break;
        }
    }

    if (axes_.size() != axisIds.size()) {
        ERROR_PRINT("MoveExcite: failed to create safe excitation for all axes\n");
        return false;
    }
    if (amplitudeAdjusted) {
        WARN_PRINT("MoveExcite: amplitude reduced from %.6f to %.6f to satisfy limits\n",
                   requestedAmplitude, amplitude_);
    }

    publishCommand(true);
    return true;
}

MoveExcite::WaveState MoveExcite::evaluateWave(double t, double phaseOffset) const
{
    WaveState state{};
    const double weight = 1.0 / static_cast<double>(kTermCount);
    for (int i = 0; i < kTermCount; ++i) {
        const double omega = 2.0 * kPi * frequencies_[i];
        const double phase = omega * t + phases_[i] + phaseOffset;
        state.wave += amplitude_ * weight * std::sin(phase);
        state.dWave += amplitude_ * weight * omega * std::cos(phase);
        state.ddWave -= amplitude_ * weight * omega * omega * std::sin(phase);
    }
    return state;
}

zrcsSystem::RunResult MoveExcite::run()
{
    const double t = static_cast<double>(tick_) * dtSec_;
    if (t > durationSec_) {
        return zrcsSystem::RunResult::SUCCESS;
    }

    double envelope = 1.0;
    double envelopeDot = 0.0;
    double envelopeDDot = 0.0;
    if (rampTimeSec_ > 0.0 && t < rampTimeSec_) {
        const double x = t / rampTimeSec_;
        envelope = smoothstep(x);
        envelopeDot = smoothstepDot01(x) / rampTimeSec_;
        envelopeDDot = smoothstepDDot01(x) / (rampTimeSec_ * rampTimeSec_);
    }

    for (auto& state : axes_) {
        const WaveState wave = evaluateWave(t, state.phaseOffset);
        const double base = state.centerOffset + wave.wave;
        double q = state.startPos + envelope * base;
        double dq = envelopeDot * base + envelope * wave.dWave;
        double ddq = envelopeDDot * base + 2.0 * envelopeDot * wave.dWave +
                     envelope * wave.ddWave;

        if (!std::isfinite(q) || !std::isfinite(dq) || !std::isfinite(ddq)) {
            ERROR_PRINT("MoveExcite: axis %d generated non-finite command\n", state.axisId);
            return zrcsSystem::RunResult::FAILED;
        }

        auto* axis = controller_->axes_[state.axisId].get();
        const double lower = axis->getNegativeLimit();
        const double upper = axis->getPositiveLimit();
        if (!clampNearLimit(q, lower, upper)) {
            ERROR_PRINT("MoveExcite: axis %d generated q %.6f exceeds [%.6f, %.6f]\n",
                        state.axisId, q, lower, upper);
            return zrcsSystem::RunResult::FAILED;
        }
        if ((q <= lower && dq < 0.0) || (q >= upper && dq > 0.0)) {
            dq = 0.0;
            ddq = 0.0;
        }
        if (std::abs(dq) > state.maxVel || std::abs(ddq) > state.maxAcc) {
            ERROR_PRINT("MoveExcite: axis %d generated dq/ddq %.6f/%.6f exceeds %.6f/%.6f\n",
                        state.axisId, dq, ddq, state.maxVel, state.maxAcc);
            return zrcsSystem::RunResult::FAILED;
        }

        axis->setAxisPositionCmd(q);
        state.lastCommandPos = q;
        state.lastCommandVel = dq;
        state.lastCommandAcc = ddq;
    }

    publishCommand(true);
    ++tick_;
    return zrcsSystem::RunResult::EXECUTING;
}

bool MoveExcite::exit()
{
    for (const auto& state : axes_) {
        if (state.axisId >= 0 && state.axisId < static_cast<int>(controller_->axes_.size())) {
            controller_->axes_[state.axisId]->setAxisPositionCmd(state.lastCommandPos);
            controller_->axes_[state.axisId]->syncCmdHistory();
        }
    }
    publishCommand(false);
    return true;
}

void MoveExcite::publishCommand(bool valid)
{
    if (!rtProcess_ || !rtProcess_->sharedBlock()) {
        return;
    }

    zrcs::MujocoIdentCommandData data{};
    zrcs::lfl_read(rtProcess_->sharedBlock()->mujocoIdentCommand, data);
    data.seq += 1;
    data.sessionId = sessionId_;
    data.axisCount = static_cast<uint32_t>(controller_->axes_.size());
    for (const auto& state : axes_) {
        if (state.axisId < 0 || state.axisId >= static_cast<int>(zrcs::kAxisMax)) {
            continue;
        }
        const uint64_t bit = 1ull << static_cast<unsigned>(state.axisId);
        if (valid) {
            data.validMask |= bit;
        } else {
            data.validMask &= ~bit;
        }
        data.qCmd[state.axisId] = state.lastCommandPos;
        data.dqCmd[state.axisId] = valid ? state.lastCommandVel : 0.0;
        data.ddqCmd[state.axisId] = valid ? state.lastCommandAcc : 0.0;
    }
    (void)commandSeq_;
    zrcs::lfl_write(rtProcess_->sharedBlock()->mujocoIdentCommand, data);
}

CMD_REGISTER(MoveExcite);

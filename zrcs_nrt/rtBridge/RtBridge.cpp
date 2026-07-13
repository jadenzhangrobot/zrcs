#include "rtBridge/RtBridge.h"

#include <algorithm>
#include <cstring>
#include <thread>

#include <spdlog/spdlog.h>

#include "config/CmdDefine.h"

RtBridge::RtBridge(zrcs::SharedBlock* block)
    : block_(block)
    , cmdProducer_(block->cmdQueue)
    , axisFbConsumer_(block->axisFeedbackQueue)
{
}

bool RtBridge::isConnected() const noexcept
{
    return block_ != nullptr;
}

std::pair<RtBridge::SendResult, uint32_t> RtBridge::sendCommand(
    const std::string& name,
    const double* args,
    size_t count)
{
    if (!block_) return {SendResult::NOT_CONNECTED, 0};

    const auto cmdId = zrcs::cmdNameToId(name);
    if (!cmdId.has_value() || *cmdId == CmdId::INVALID)
    {
        spdlog::error("[RtBridge] Unknown command '{}', not registered in cmdNameToId", name);
        return {SendResult::UNKNOWN_CMD, 0};
    }

    zrcs::Command cmd{};
    cmd.cmdId = static_cast<uint16_t>(*cmdId);
    cmd.seq   = seq_counter_.fetch_add(1, std::memory_order_relaxed);

    const size_t n = std::min(count, zrcs::kCmdArgsMax);
    if (args && n > 0) std::memcpy(cmd.args, args, n * sizeof(double));
    {
        std::lock_guard<std::mutex> lk(push_mutex_);
        if (!cmdProducer_.push(cmd))
        {
            ++dropped_count_;
            spdlog::error("[RtBridge] Queue full, dropped '{}' (seq={})", name, cmd.seq);
            return {SendResult::QUEUE_FULL, 0};
        }
    }

    spdlog::info("[RtBridge] Sent '{}' seq={} cmdId={} head_after={}",
                 name, cmd.seq, cmd.cmdId,
                 block_->cmdQueue.head.load(std::memory_order_relaxed));
    return {SendResult::OK, cmd.seq};
}

std::pair<RtBridge::SendResult, uint32_t> RtBridge::sendCommand(
    const std::string& name,
    const std::vector<double>& args)
{
    return sendCommand(name, args.data(), args.size());
}

std::pair<RtBridge::SendResult, uint32_t> RtBridge::sendCommand(
    const std::string& name,
    const std::string& csv_args)
{
    if (csv_args.empty()) return sendCommand(name, nullptr, 0);

    double buf[zrcs::kCmdArgsMax]{};
    size_t idx = 0;
    size_t pos = 0;
    while (idx < zrcs::kCmdArgsMax)
    {
        size_t comma = csv_args.find(',', pos);
        const std::string token = csv_args.substr(pos, comma - pos);
        try { buf[idx++] = std::stod(token); } catch (...) { break; }
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return sendCommand(name, buf, idx);
}

bool RtBridge::readLatestAxisPositions(zrcs::JointPosData& out) const noexcept
{
    if (!block_) return false;
    return zrcs::lfl_read(block_->axisPositions, out);
}

bool RtBridge::readLatestAxisFeedback(zrcs::AxisFeedbackData& out) noexcept
{
    return axisFbConsumer_.pop(out);
}

bool RtBridge::readMujocoIdentStatus(zrcs::MujocoIdentStatusData& out) const noexcept
{
    if (!block_) return false;
    return zrcs::lfl_read(block_->mujocoIdentStatus, out);
}

uint8_t RtBridge::axisCount() const noexcept
{
    if (!block_) return 0;
    return block_->axisCount.load(std::memory_order_acquire);
}

bool RtBridge::readFkResult(zrcs::FkResultData& out) const noexcept
{
    if (!block_) return false;
    return zrcs::lfl_read(block_->fkResult, out);
}

bool RtBridge::readJointPosResult(zrcs::JointPosData& out) const noexcept
{
    if (!block_) return false;
    return zrcs::lfl_read(block_->jointPosResult, out);
}

bool RtBridge::readProbeResult(zrcs::ProbeResultData& out) const noexcept
{
    if (!block_) return false;
    return zrcs::lfl_read(block_->probeResult, out);
}

bool RtBridge::readCaptureResult(zrcs::CaptureData& out) const noexcept
{
    if (!block_) return false;
    return zrcs::lfl_read(block_->captureResult, out);
}

bool RtBridge::probeTriggered() const noexcept
{
    if (!block_) return false;
    return block_->probeTriggered.load(std::memory_order_acquire);
}

bool RtBridge::captureTriggered() const noexcept
{
    if (!block_) return false;
    return block_->captureTriggered.load(std::memory_order_acquire);
}

uint32_t RtBridge::ioReadResult() const noexcept
{
    if (!block_) return 0;
    return block_->ioReadResult.load(std::memory_order_acquire);
}

void RtBridge::setTaskScheduling(zrcs::TaskScheduling ts) noexcept
{
    if (block_) block_->taskSched.store(ts, std::memory_order_release);
}

zrcs::TaskScheduling RtBridge::getTaskScheduling() const noexcept
{
    if (!block_) return zrcs::TaskScheduling::STOP;
    return block_->taskSched.load(std::memory_order_acquire);
}

void RtBridge::requestRun()
{
    setTaskScheduling(zrcs::TaskScheduling::RUN);
}

void RtBridge::requestStop()
{
    stopContinuousMotion();
    setTaskScheduling(zrcs::TaskScheduling::STOP);
}

void RtBridge::requestReset()
{
    stopContinuousMotion();
    setTaskScheduling(zrcs::TaskScheduling::RESET);
}

void RtBridge::requestStart()
{
    setTaskScheduling(zrcs::TaskScheduling::IDLE);
}

void RtBridge::requestShutdown()
{
    setTaskScheduling(zrcs::TaskScheduling::SHUTDOWN);
}

uint64_t RtBridge::heartbeat() const noexcept
{
    if (!block_) return 0;
    uint64_t value = 0;
    zrcs::lfl_read(block_->heartbeat, value);
    return value;
}

bool RtBridge::isRtAlive() noexcept
{
    const uint64_t cur = heartbeat();
    const bool alive = (cur != prev_heartbeat_);
    prev_heartbeat_ = cur;
    return alive;
}

void RtBridge::setSpeedMultiplier(uint8_t percent) noexcept
{
    if (!block_) return;
    block_->overrideRatio.store(
        std::clamp(percent / 100.0, 0.0, 1.0), std::memory_order_release);
}

uint8_t RtBridge::speedMultiplier() const noexcept
{
    if (!block_) return 0;
    return static_cast<uint8_t>(
        block_->overrideRatio.load(std::memory_order_acquire) * 100.0);
}

void RtBridge::startContinuousMotion(int axisId, bool direction) noexcept
{
    if (!block_) return;
    block_->jogCtrl.axisId.store(axisId, std::memory_order_relaxed);
    block_->jogCtrl.direction.store(direction, std::memory_order_relaxed);
    block_->jogCtrl.active.store(true, std::memory_order_release);
}

void RtBridge::stopContinuousMotion() noexcept
{
    if (block_) block_->jogCtrl.active.store(false, std::memory_order_release);
}

RtBridge::CmdCompletion RtBridge::lastCompletion() const noexcept
{
    if (!block_) return {0, false};
    const auto completion = zrcs::unpackCmdCompletion(
        block_->lastCmdCompletion.load(std::memory_order_acquire));
    return {completion.seq, completion.result == 0};
}

bool RtBridge::isCommandCompleted(uint32_t seq) const noexcept
{
    if (!block_) return false;
    const auto completion = zrcs::unpackCmdCompletion(
        block_->lastCmdCompletion.load(std::memory_order_acquire));
    return completion.seq >= seq;
}

bool RtBridge::waitForCompletion(uint32_t seq, std::chrono::milliseconds timeout) noexcept
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (isCommandCompleted(seq)) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

void RtBridge::setConfJ(bool enabled) noexcept
{
    if (block_) block_->confJEnabled.store(enabled, std::memory_order_release);
}

void RtBridge::setConfL(bool enabled) noexcept
{
    if (block_) block_->confLEnabled.store(enabled, std::memory_order_release);
}

void RtBridge::setSingAreaMode(uint8_t mode) noexcept
{
    if (block_) block_->singAreaMode.store(mode, std::memory_order_release);
}

void RtBridge::setPathMoveConfig(double maxVel, double maxAccel, double maxJerk) noexcept
{
    if (!block_) return;
    block_->pathMoveCfg.maxVel.store(maxVel, std::memory_order_release);
    block_->pathMoveCfg.maxAccel.store(maxAccel, std::memory_order_release);
    block_->pathMoveCfg.maxJerk.store(maxJerk, std::memory_order_release);
}

void RtBridge::setGalvoConfig(int platXId, int platYId, int galvoXId, int galvoYId,
                              double cutoffHz) noexcept
{
    if (!block_) return;
    block_->galvoCfg.platXId.store(platXId, std::memory_order_release);
    block_->galvoCfg.platYId.store(platYId, std::memory_order_release);
    block_->galvoCfg.galvoXId.store(galvoXId, std::memory_order_release);
    block_->galvoCfg.galvoYId.store(galvoYId, std::memory_order_release);
    block_->galvoCfg.cutoffHz.store(cutoffHz, std::memory_order_release);
}

uint64_t RtBridge::droppedCount() const noexcept
{
    return dropped_count_.load(std::memory_order_relaxed);
}

#include "command/TaskService.h"

#include <spdlog/spdlog.h>

#include "command/CommandService.h"
#include "rtBridge/RtBridge.h"

TaskService::TaskService(RtBridge* bridge, CommandService* commands)
    : bridge_(bridge)
    , commands_(commands)
{
    buildHandlers();
}

bool TaskService::isTaskCommand(const std::string& name) const
{
    return handlers_.find(name) != handlers_.end();
}

bool TaskService::tryHandle(const std::string& name,
                            const std::vector<double>& args,
                            std::string& reply)
{
    auto it = handlers_.find(name);
    if (it == handlers_.end()) {
        return false;
    }
    reply = it->second(args);
    return true;
}

void TaskService::buildHandlers()
{
    handlers_["SYS_RUN"] = [this](const std::vector<double>&) {
        bridge_->requestRun();
        spdlog::info("[TaskService] SYS_RUN -> RUN");
        return std::string("OK");
    };
    handlers_["SYS_STOP"] = [this](const std::vector<double>&) {
        bridge_->requestStop();
        spdlog::info("[TaskService] SYS_STOP -> STOP");
        return std::string("OK");
    };
    handlers_["SYS_RESET"] = [this](const std::vector<double>&) {
        bridge_->requestReset();
        spdlog::info("[TaskService] SYS_RESET -> RESET");
        return std::string("OK");
    };
    handlers_["SYS_ESTOP"] = [this](const std::vector<double>&) {
        bridge_->requestStop();
        spdlog::warn("[TaskService] SYS_ESTOP -> STOP");
        return std::string("OK");
    };
    handlers_["SYS_JOG_START"] = [this](const std::vector<double>& args) {
        if (args.size() < 2) {
            return std::string("ERROR: SYS_JOG_START requires 2 args (axisId, direction)");
        }
        const int axisId = static_cast<int>(args[0]);
        const bool direction = args[1] > 0.0;
        bridge_->startContinuousMotion(axisId, direction);
        spdlog::info("[TaskService] SYS_JOG_START: axis={}, dir={}", axisId, direction);
        return std::string("OK");
    };
    handlers_["SYS_JOG_STOP"] = [this](const std::vector<double>&) {
        bridge_->stopContinuousMotion();
        spdlog::info("[TaskService] SYS_JOG_STOP");
        return std::string("OK");
    };
    handlers_["SYS_SET_MULTIPLIER"] = [this](const std::vector<double>& args) {
        if (args.empty()) {
            return std::string("ERROR: SYS_SET_MULTIPLIER requires 1 arg (percent)");
        }
        bridge_->setSpeedMultiplier(static_cast<uint8_t>(args[0]));
        spdlog::info("[TaskService] SYS_SET_MULTIPLIER: {}%", static_cast<int>(args[0]));
        return std::string("OK");
    };
    handlers_["SYS_SET_ORIGIN"] = [this](const std::vector<double>&) {
        const double sentinel = static_cast<double>(bridge_->axisCount());
        if (!commands_) {
            return std::string("ERROR: Not connected");
        }
        const auto [result, seq] =
            commands_->submitWithSeq("SetZero", std::vector<double>{sentinel});
        if (result != RtBridge::SendResult::OK) {
            spdlog::error("[TaskService] SYS_SET_ORIGIN failed");
            return std::string("ERROR: Queue full");
        }
        spdlog::info("[TaskService] SYS_SET_ORIGIN -> SetZero(all) seq={}", seq);
        return std::string("OK");
    };
    handlers_["SYS_SET_AXIS_ORIGIN"] = [this](const std::vector<double>& args) {
        if (args.empty()) {
            return std::string("ERROR: SYS_SET_AXIS_ORIGIN requires 1 arg (axisId)");
        }
        if (!commands_) {
            return std::string("ERROR: Not connected");
        }
        const auto [result, seq] = commands_->submitWithSeq("SetZero", args);
        if (result != RtBridge::SendResult::OK) {
            spdlog::error("[TaskService] SYS_SET_AXIS_ORIGIN failed");
            return std::string("ERROR: Queue full");
        }
        spdlog::info("[TaskService] SYS_SET_AXIS_ORIGIN -> SetZero(axis={}) seq={}",
                     static_cast<int>(args[0]),
                     seq);
        return std::string("OK");
    };
}

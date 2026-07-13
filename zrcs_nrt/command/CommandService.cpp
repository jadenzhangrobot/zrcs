#include "command/CommandService.h"

#include <spdlog/spdlog.h>

CommandService::CommandService(RtBridge* bridge)
    : bridge_(bridge)
{
}

std::pair<RtBridge::SendResult, uint32_t> CommandService::submitWithSeq(
    const std::string& name, const std::vector<double>& args)
{
    if (!bridge_) {
        return {RtBridge::SendResult::NOT_CONNECTED, 0};
    }
    return bridge_->sendCommand(name, args);
}

std::string CommandService::submit(const std::string& name, const std::vector<double>& args)
{
    const auto [result, seq] = submitWithSeq(name, args);
    switch (result) {
    case RtBridge::SendResult::OK:
        spdlog::info("[CommandService] Command '{}' sent via RtBridge (seq={})", name, seq);
        return "OK";
    case RtBridge::SendResult::UNKNOWN_CMD:
        spdlog::error("[CommandService] Unknown command '{}'", name);
        return "ERROR: Unknown command";
    case RtBridge::SendResult::QUEUE_FULL:
        spdlog::error("[CommandService] Command queue full! cmd='{}'", name);
        return "ERROR: Queue full";
    default:
        spdlog::error("[CommandService] Command '{}' failed: bridge not connected", name);
        return "ERROR: Not connected";
    }
}

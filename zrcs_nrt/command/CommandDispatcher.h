#pragma once
/**
 * @file CommandDispatcher.h
 * @brief NRT 命令分发的领域层。
 *
 * 职责：把"命令名 + 参数"翻译成对 RtBridge 的具体调用，并返回给上位机的回复串。
 * 分两类命令：
 *   - 系统命令（SYS_*）：直接调用 RtBridge 的调度/点动/倍率等方法，不走 cmdQueue。
 *                        清单集中在 buildSystemHandlers() 的注册表里，加一条即新增一个。
 *   - 运动命令（MoveL/MoveJ/...）：未命中系统命令表时统一透传 bridge_->sendCommand()，
 *                        由 CmdDefine.h 的命令表 + RtBridge 的 magic_enum 校验决定合法性。
 *
 * 设计意图：让 ZmqServer 退化为纯传输层（protobuf 解码 + 回复），
 *          所有业务语义（如 SYS_SET_ORIGIN 用 axisCount 作哨兵）都收敛到此处，
 *          从而可脱离 ZMQ 单独测试，并被其它入口（如 BT）复用。
 */

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <spdlog/spdlog.h>

#include "command/RtBridge.h"

class CommandDispatcher
{
public:
    explicit CommandDispatcher(RtBridge* bridge)
        : bridge_(bridge)
    {
        buildSystemHandlers();
    }

    CommandDispatcher(const CommandDispatcher&) = delete;
    CommandDispatcher& operator=(const CommandDispatcher&) = delete;

    /// 分发一条命令，返回给上位机的回复字符串（"OK" / "ERROR: ..."）。
    std::string dispatch(const std::string& name, const std::vector<double>& args)
    {
        auto it = systemHandlers_.find(name);
        if (it != systemHandlers_.end())
        {
            return it->second(args);
        }
        return dispatchMotionCommand(name, args);
    }

private:
    using Handler = std::function<std::string(const std::vector<double>&)>;

    /// 运动命令路径：透传给 RtBridge，由 magic_enum 校验命令名合法性。
    std::string dispatchMotionCommand(const std::string& name,
                                      const std::vector<double>& args)
    {
        const auto [result, seq] = bridge_->sendCommand(name, args);
        switch (result)
        {
        case RtBridge::SendResult::OK:
            spdlog::info("[Dispatcher] Command '{}' sent via RtBridge (seq={})", name, seq);
            return "OK";
        case RtBridge::SendResult::UNKNOWN_CMD:
            spdlog::error("[Dispatcher] Unknown command '{}'", name);
            return "ERROR: Unknown command";
        case RtBridge::SendResult::QUEUE_FULL:
            spdlog::error("[Dispatcher] Command queue full! cmd='{}'", name);
            return "ERROR: Queue full";
        default:
            spdlog::error("[Dispatcher] Command '{}' failed: bridge not connected", name);
            return "ERROR: Not connected";
        }
    }

    /// 构建系统命令注册表。新增一个 SYS_* 命令 = 这里加一条 systemHandlers_[...]。
    void buildSystemHandlers()
    {
        systemHandlers_["SYS_RUN"] = [this](const std::vector<double>&) {
            bridge_->requestRun();
            spdlog::info("[Dispatcher] SYS_RUN -> RUN");
            return std::string("OK");
        };
        systemHandlers_["SYS_STOP"] = [this](const std::vector<double>&) {
            bridge_->requestStop();
            spdlog::info("[Dispatcher] SYS_STOP -> STOP");
            return std::string("OK");
        };
        systemHandlers_["SYS_RESET"] = [this](const std::vector<double>&) {
            bridge_->requestReset();
            spdlog::info("[Dispatcher] SYS_RESET -> RESET");
            return std::string("OK");
        };
        systemHandlers_["SYS_ESTOP"] = [this](const std::vector<double>&) {
            bridge_->requestStop();
            spdlog::warn("[Dispatcher] SYS_ESTOP -> STOP");
            return std::string("OK");
        };
        systemHandlers_["SYS_JOG_START"] = [this](const std::vector<double>& args) {
            if (args.size() < 2)
                return std::string("ERROR: SYS_JOG_START requires 2 args (axisId, direction)");
            const int axisId = static_cast<int>(args[0]);
            const bool direction = args[1] > 0.0;
            bridge_->startContinuousMotion(axisId, direction);
            spdlog::info("[Dispatcher] SYS_JOG_START: axis={}, dir={}", axisId, direction);
            return std::string("OK");
        };
        systemHandlers_["SYS_JOG_STOP"] = [this](const std::vector<double>&) {
            bridge_->stopContinuousMotion();
            spdlog::info("[Dispatcher] SYS_JOG_STOP");
            return std::string("OK");
        };
        systemHandlers_["SYS_SET_MULTIPLIER"] = [this](const std::vector<double>& args) {
            if (args.empty())
                return std::string("ERROR: SYS_SET_MULTIPLIER requires 1 arg (percent)");
            bridge_->setSpeedMultiplier(static_cast<uint8_t>(args[0]));
            spdlog::info("[Dispatcher] SYS_SET_MULTIPLIER: {}%", static_cast<int>(args[0]));
            return std::string("OK");
        };
        // SetZero 全轴：以 axisCount 作为哨兵值传入
        systemHandlers_["SYS_SET_ORIGIN"] = [this](const std::vector<double>&) {
            const double sentinel = static_cast<double>(bridge_->axisCount());
            const auto [result, seq] = bridge_->sendCommand("SetZero", std::vector<double>{sentinel});
            if (result != RtBridge::SendResult::OK)
            {
                spdlog::error("[Dispatcher] SYS_SET_ORIGIN failed");
                return std::string("ERROR: Queue full");
            }
            spdlog::info("[Dispatcher] SYS_SET_ORIGIN -> SetZero(all) seq={}", seq);
            return std::string("OK");
        };
        // SetZero 指定轴：直接把 axisId 参数透传给 SetZero
        systemHandlers_["SYS_SET_AXIS_ORIGIN"] = [this](const std::vector<double>& args) {
            if (args.empty())
                return std::string("ERROR: SYS_SET_AXIS_ORIGIN requires 1 arg (axisId)");
            const auto [result, seq] = bridge_->sendCommand("SetZero", args);
            if (result != RtBridge::SendResult::OK)
            {
                spdlog::error("[Dispatcher] SYS_SET_AXIS_ORIGIN failed");
                return std::string("ERROR: Queue full");
            }
            spdlog::info("[Dispatcher] SYS_SET_AXIS_ORIGIN -> SetZero(axis={}) seq={}",
                         static_cast<int>(args[0]), seq);
            return std::string("OK");
        };
    }

    RtBridge* bridge_;
    std::unordered_map<std::string, Handler> systemHandlers_;
};

#pragma once
/**
 * @file CommandDispatcher.h
 * @brief NRT 命令分发的领域层。
 *
 * 职责：把"命令名 + 参数"翻译成对 RtBridge 的具体调用，并返回给上位机的回复串。
 * 分两类命令：
 *   - 系统命令（SYS_*）：直接调用 RtBridge 的调度/点动/倍率等方法，不走 cmdQueue。
 *   - 运动命令（MoveL/MoveJ/...）：未命中系统命令表时统一透传 bridge_->sendCommand()。
 */

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class RtBridge;

class CommandDispatcher
{
public:
    explicit CommandDispatcher(RtBridge* bridge);

    CommandDispatcher(const CommandDispatcher&) = delete;
    CommandDispatcher& operator=(const CommandDispatcher&) = delete;

    /// 分发一条命令，返回给上位机的回复字符串（"OK" / "ERROR: ..."）。
    std::string dispatch(const std::string& name, const std::vector<double>& args);

private:
    using Handler = std::function<std::string(const std::vector<double>&)>;

    std::string dispatchMotionCommand(const std::string& name,
                                      const std::vector<double>& args);
    void buildSystemHandlers();

    RtBridge* bridge_;
    std::unordered_map<std::string, Handler> systemHandlers_;
};

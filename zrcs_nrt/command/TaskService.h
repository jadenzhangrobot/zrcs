#pragma once
/**
 * @file TaskService.h
 * @brief 系统任务命令：RUN/STOP/RESET/点动/倍率/设原点等（不经 cmdQueue 或特殊包装）。
 */

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class RtBridge;
class CommandService;

class TaskService {
public:
    TaskService(RtBridge* bridge, CommandService* commands);

    /// 若 name 是系统任务命令则处理并返回 true，reply 为回复串。
    bool tryHandle(const std::string& name,
                   const std::vector<double>& args,
                   std::string& reply);

    /// 是否为系统任务命令名。
    bool isTaskCommand(const std::string& name) const;

private:
    using Handler = std::function<std::string(const std::vector<double>&)>;

    void buildHandlers();

    RtBridge* bridge_;
    CommandService* commands_;
    std::unordered_map<std::string, Handler> handlers_;
};

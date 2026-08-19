#pragma once
/**
 * @file BtSharedState.h
 * @brief 行为树节点与 Runner 之间的共享状态（线程安全）。
 */

#include <mutex>
#include <string>
#include <utility>

class RtBridge;
class CommandService;

namespace zrcs_bt {

struct SharedState
{
    RtBridge* bridge = nullptr;         ///< 仅用于查询（isCommandCompleted 等）
    CommandService* commands = nullptr; ///< 发令统一入口
    mutable std::mutex mutex;
    std::string currentNode;
    std::string message;

    /// 设置当前节点名和描述文本（线程安全）。
    void setCurrentNode(const std::string& nodeName, const std::string& text);

    /// 获取当前状态的快照（线程安全）。
    std::pair<std::string, std::string> snapshot() const;
};

} // namespace zrcs_bt

#pragma once
/**
 * @file BtSharedState.h
 * @brief 行为树节点与 Runner 之间的共享状态（线程安全）。
 *
 * 每个节点通过 shared_ptr 持有同一个 SharedState 实例，
 * 用于在执行过程中更新当前节点名和状态消息。
 */

#include <mutex>
#include <string>
#include <utility>

class RtBridge;

namespace zrcs_bt {

struct SharedState
{
    RtBridge* bridge = nullptr;
    mutable std::mutex mutex;
    std::string currentNode;
    std::string message;

    /// 设置当前节点名和描述文本（线程安全）。
    void setCurrentNode(const std::string& nodeName, const std::string& text)
    {
        std::lock_guard<std::mutex> lock(mutex);
        currentNode = nodeName;
        message = text;
    }

    /// 获取当前状态的快照（线程安全）。
    std::pair<std::string, std::string> snapshot() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return {currentNode, message};
    }
};

} // namespace zrcs_bt

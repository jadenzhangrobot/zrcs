#include "behavior_tree/core/BtSharedState.h"

namespace zrcs_bt {

void SharedState::setCurrentNode(const std::string& nodeName, const std::string& text)
{
    std::lock_guard<std::mutex> lock(mutex);
    currentNode = nodeName;
    message = text;
}

std::pair<std::string, std::string> SharedState::snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return {currentNode, message};
}

} // namespace zrcs_bt

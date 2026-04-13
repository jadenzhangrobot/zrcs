#include "system/NodeFactory.h"
#include "CmdRegistry_gen.h"
#include <cstring>

namespace zrcsSystem {

std::vector<NodeFactory::PendingCmd>& NodeFactory::pendingCmds()
{
    static std::vector<PendingCmd> v;
    return v;
}

std::vector<NodeFactory::PendingCmdById>& NodeFactory::pendingCmdsById()
{
    static std::vector<PendingCmdById> v;
    return v;
}

std::vector<NodeFactory::PendingOutput>& NodeFactory::pendingOutputs()
{
    static std::vector<PendingOutput> v;
    return v;
}

std::vector<NodeFactory::PendingInput>& NodeFactory::pendingInputs()
{
    static std::vector<PendingInput> v;
    return v;
}

NodeFactory::NodeFactory()
{
    // 旧路径：按名称反查 CmdId（兼容过渡期，未来移除）
    for (auto& p : pendingCmds()) {
        for (size_t i = 1; i < static_cast<size_t>(CmdId::SENTINEL); ++i) {
            if (p.name == zrcs::cmdIdToName(static_cast<uint16_t>(i))) {
                registry_[i] = p.creator;
                break;
            }
        }
    }

    // 新路径：按 CmdId 直接注册，O(N)，无字符串匹配
    for (auto& p : pendingCmdsById()) {
        if (p.cmdId > 0 && p.cmdId < static_cast<uint16_t>(CmdId::SENTINEL)) {
            registry_[p.cmdId] = p.creator;
        }
    }

    for (auto& p : pendingOutputs())
        outPutNodes.push_back(p.node);

    for (auto& p : pendingInputs())
        inPutNodes.push_back(p.node);
}

bool NodeFactory::exist(std::string_view name) const
{
    for (size_t i = 1; i < static_cast<size_t>(CmdId::SENTINEL); ++i) {
        if (registry_[i] && name == zrcs::cmdIdToName(static_cast<uint16_t>(i)))
            return true;
    }
    return false;
}

} // namespace zrcsSystem

#include "system/NodeFactory.h"
#include "config/CmdDefine.h"
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
    for (auto& p : pendingCmdsById()) {
        const auto idx = static_cast<size_t>(p.cmdId);
        if (idx > 0 && idx < static_cast<size_t>(CmdId::SENTINEL)) {
            registry_[idx] = p.creator;
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

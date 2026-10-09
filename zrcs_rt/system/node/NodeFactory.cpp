#include "system/node/NodeFactory.h"
#include "config/CmdDefine.h"
#include <algorithm>
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
        outputPeriodics.push_back(p.node);

    for (auto& p : pendingInputs())
        inputPeriodics.push_back(p.node);
}

void NodeFactory::sortPeriodics()
{
    const auto byOrder = [](const Periodic& a, const Periodic& b) {
        return a->execOrder_ < b->execOrder_;
    };
    std::stable_sort(inputPeriodics.begin(),  inputPeriodics.end(),  byOrder);
    std::stable_sort(outputPeriodics.begin(), outputPeriodics.end(), byOrder);
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

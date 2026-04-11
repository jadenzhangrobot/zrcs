#include "system/NodeFactory.h"

namespace zrcsSystem {

std::vector<NodeFactory::PendingCmd>& NodeFactory::pendingCmds()
{
    static std::vector<PendingCmd> v;
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
    for (auto& p : pendingCmds())
        registry_[p.name] = p.creator;

    for (auto& p : pendingOutputs())
        outPutNodes.push_back(p.node);

    for (auto& p : pendingInputs())
        inPutNodes.push_back(p.node);
}

NodeFactory::Creator NodeFactory::getNodePtr(std::string_view name)
{
    auto it = registry_.find(name);
    return (it != registry_.end()) ? it->second : nullptr;
}

bool NodeFactory::exist(std::string_view name) const
{
    return registry_.count(name) > 0;
}

} // namespace zrcsSystem

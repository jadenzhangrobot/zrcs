#include "system/NodeFactory.h"
#include "config/CmdArgs.h"
#include <cstring>

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
    // 将 pending 命令按名称反查 CmdId，填入数组
    for (auto& p : pendingCmds()) {
        // 扫描 cmdIdToName 表找到匹配的 CmdId
        for (size_t i = 1; i < static_cast<size_t>(CmdId::SENTINEL); ++i) {
            if (p.name == zrcs::cmdIdToName(static_cast<uint16_t>(i))) {
                registry_[i] = p.creator;
                break;
            }
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

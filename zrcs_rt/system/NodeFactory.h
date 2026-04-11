#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include "system/base/BaseNodeInterface.h"

class ModelRegistry;

namespace zrcsSystem {

class NodeFactory {
public:
    using Creator = std::shared_ptr<CmdNode>;
    using Output  = std::shared_ptr<OutputNode>;
    using Input   = std::shared_ptr<InputNode>;

    std::vector<Output> outPutNodes;
    std::vector<Input>  inPutNodes;
    ZrcsHardware::Controller* control       = nullptr;
    RTProcess*                rtProcess     = nullptr;
    ModelRegistry*            modelRegistry = nullptr;

    // 构造时从静态 pending 列表接管所有注册
    NodeFactory();

    Creator getNodePtr(std::string_view name);
    bool    exist(std::string_view name) const;

    // 静态 pending 列表 —— 仅供注册宏使用，函数内静态避免初始化顺序问题
    struct PendingCmd    { std::string_view name; Creator creator; };
    struct PendingOutput { Output node; };
    struct PendingInput  { Input  node; };

    static std::vector<PendingCmd>&    pendingCmds();
    static std::vector<PendingOutput>& pendingOutputs();
    static std::vector<PendingInput>&  pendingInputs();

    ~NodeFactory() = default;

private:
    std::unordered_map<std::string_view, Creator> registry_;
};

// ── 注册辅助模板（写 pending，不再依赖单例）────────────────────

template <typename T>
class RegisterNode {
public:
    explicit RegisterNode(std::string_view name) {
        NodeFactory::pendingCmds().push_back({name, std::make_shared<T>()});
    }
};

template <typename T>
class registerAndAddOutputNode {
public:
    registerAndAddOutputNode() {
        NodeFactory::pendingOutputs().push_back({std::make_shared<T>()});
    }
};

template <typename T>
class registerAndAddInputNode {
public:
    registerAndAddInputNode() {
        NodeFactory::pendingInputs().push_back({std::make_shared<T>()});
    }
};

} // namespace zrcsSystem

#define REGISTERCMD(className) \
    static zrcsSystem::RegisterNode<className> register##className(#className);

#define REGISTEROUTPUT(className) \
    static zrcsSystem::registerAndAddOutputNode<className> register_##className;

#define REGISTERINPUT(className) \
    static zrcsSystem::registerAndAddInputNode<className> register_##className;

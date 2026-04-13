#pragma once

#include <memory>
#include <array>
#include <string_view>
#include <vector>
#include "system/base/BaseNodeInterface.h"
#include "CmdRegistry_gen.h"

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

    // 按 CmdId 直接索引，O(1)，无字符串转换
    Creator getNodePtr(CmdId id) const noexcept {
        const auto idx = static_cast<size_t>(id);
        if (idx == 0 || idx >= static_cast<size_t>(CmdId::SENTINEL)) return nullptr;
        return registry_[idx];
    }

    bool exist(std::string_view name) const;

    // 静态 pending 列表 —— 仅供注册宏使用，函数内静态避免初始化顺序问题
    struct PendingCmd    { std::string_view name; Creator creator; };
    struct PendingCmdById { uint16_t cmdId; Creator creator; };
    struct PendingOutput { Output node; };
    struct PendingInput  { Input  node; };

    static std::vector<PendingCmd>&    pendingCmds();
    static std::vector<PendingCmdById>& pendingCmdsById();
    static std::vector<PendingOutput>& pendingOutputs();
    static std::vector<PendingInput>&  pendingInputs();

    ~NodeFactory() = default;

private:
    // 下标 = CmdId 枚举值，0(INVALID) 和越界位置为 nullptr
    std::array<Creator, static_cast<size_t>(CmdId::SENTINEL)> registry_{};
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
class RegisterNodeById {
public:
    explicit RegisterNodeById(uint16_t cmdId) {
        NodeFactory::pendingCmdsById().push_back({cmdId, std::make_shared<T>()});
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

// REGISTERCMD — 按 CmdId 直接注册（推荐）
#define REGISTERCMD(className, id) \
    static zrcsSystem::RegisterNodeById<className> register##className(id);

#define REGISTEROUTPUT(className) \
    static zrcsSystem::registerAndAddOutputNode<className> register_##className;

#define REGISTERINPUT(className) \
    static zrcsSystem::registerAndAddInputNode<className> register_##className;

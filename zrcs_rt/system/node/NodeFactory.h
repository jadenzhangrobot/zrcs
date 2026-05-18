/**
 * @file NodeFactory.h
 * @brief Node factory and static-registration helpers.
 *
 * Command, output, and input nodes are registered via macros (CMD_REGISTER,
 * REGISTEROUTPUT, REGISTERINPUT) that write shared_ptr instances into
 * function-local static pending lists.  At construction, the NodeFactory
 * drains those lists into its internal registries.
 *
 * Lookup is O(1): command nodes are indexed directly by CmdId enum value.
 */

#pragma once

#include <array>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "config/CmdDefine.h"
#include "system/node/BaseNodeInterface.h"

class ModelRegistry;

namespace zrcsSystem {

/**
 * @brief Central registry and factory for all node types.
 *
 * Drains static pending lists (populated by the registration macros) on
 * construction.  Provides O(1) command-node lookup by CmdId and named
 * existence checks.
 */
class NodeFactory {
public:
    using Creator = std::shared_ptr<CmdNode>;
    using Output  = std::shared_ptr<OutputNode>;
    using Input   = std::shared_ptr<InputNode>;

    /// Registered output nodes (executed in insertion order).
    std::vector<Output> outPutNodes;
    /// Registered input nodes (executed in insertion order).
    std::vector<Input>  inPutNodes;

    /// Back-pointers injected after construction by NodeManager.
    ZrcsHardware::Controller* control       = nullptr;
    RTProcess*                rtProcess     = nullptr;
    ModelRegistry*            modelRegistry = nullptr;

    /// Constructor -- drains all static pending lists into the registries.
    NodeFactory();

    /// Look up a command node by @p id.  Returns nullptr for INVALID or
    /// out-of-range ids.  Complexity: O(1).
    Creator getNodePtr(CmdId id) const noexcept
    {
        const auto idx = static_cast<size_t>(id);
        if (idx == 0 || idx >= static_cast<size_t>(CmdId::SENTINEL)) {
            return nullptr;
        }
        return registry_[idx];
    }

    /// Check whether a command with the given @p name has been registered.
    bool exist(std::string_view name) const;

    // ---- Static pending lists (populated by registration macros) ------------
    // Function-local statics avoid static-initialisation-order issues.

    /// Pending registration by (name, creator) pair.
    struct PendingCmd    { std::string_view name; Creator creator; };
    /// Pending registration by (CmdId, creator) pair -- preferred path.
    struct PendingCmdById { CmdId cmdId; Creator creator; };
    /// Pending output-node registration.
    struct PendingOutput { Output node; };
    /// Pending input-node registration.
    struct PendingInput  { Input  node; };

    static std::vector<PendingCmd>&     pendingCmds();
    static std::vector<PendingCmdById>& pendingCmdsById();
    static std::vector<PendingOutput>&  pendingOutputs();
    static std::vector<PendingInput>&   pendingInputs();

    ~NodeFactory() = default;

private:
    /// Index = CmdId enum value.  Slot 0 (INVALID) and out-of-range indices
    /// always contain nullptr.
    std::array<Creator, static_cast<size_t>(CmdId::SENTINEL)> registry_{};
};

// ===========================================================================
// Registration helper templates
//
// Each helper writes a shared_ptr<T> into the appropriate pending list during
// static initialisation.  NodeFactory's constructor drains these lists.
// ===========================================================================

/**
 * @brief Register a CmdNode subclass by name (legacy path).
 * @tparam T CmdNode subclass to register.
 */
template <typename T>
class RegisterNode {
public:
    explicit RegisterNode(std::string_view name)
    {
        NodeFactory::pendingCmds().push_back(NodeFactory::PendingCmd{name, std::make_shared<T>()});
    }
};

/**
 * @brief Register a CmdNode subclass by CmdId (preferred path).
 * @tparam T CmdNode subclass to register.
 */
template <typename T>
class RegisterNodeById {
public:
    explicit RegisterNodeById(CmdId cmdId)
    {
        NodeFactory::pendingCmdsById().push_back(
            NodeFactory::PendingCmdById{cmdId, std::make_shared<T>()});
    }
};

/**
 * @brief Register an OutputNode subclass.
 * @tparam T OutputNode subclass to register.
 */
template <typename T>
class registerAndAddOutputNode {
public:
    registerAndAddOutputNode()
    {
        NodeFactory::pendingOutputs().push_back({std::make_shared<T>()});
    }
};

/**
 * @brief Register an InputNode subclass.
 * @tparam T InputNode subclass to register.
 */
template <typename T>
class registerAndAddInputNode {
public:
    registerAndAddInputNode()
    {
        NodeFactory::pendingInputs().push_back({std::make_shared<T>()});
    }
};

} // namespace zrcsSystem

// ===========================================================================
// Free helpers and registration macros
// ===========================================================================

/**
 * @brief Resolve a command name to its CmdId; throws on failure.
 *
 * Used by CMD_REGISTER to map the class name (via magic_enum) to a CmdId.
 *
 * @param name Command name (== class name by convention).
 * @return Corresponding CmdId.
 * @throws std::logic_error if the name is unknown.
 */
inline CmdId resolveCmdIdOrThrow(std::string_view name)
{
    const auto cmdId = ::cmdNameToId(name);
    if (!cmdId.has_value()) {
        throw std::logic_error("Unknown command name in CMD_REGISTER");
    }
    return *cmdId;
}

/**
 * @def CMD_REGISTER(className)
 * @brief Register a CmdNode subclass.  The class name is used as the
 *        command name and resolved to a CmdId via magic_enum.
 */
#define CMD_REGISTER(className) \
    static zrcsSystem::RegisterNodeById<class className> register##className( \
        ::resolveCmdIdOrThrow(#className));

/**
 * @def REGISTERCMD(className, id)
 * @brief Legacy alias for CMD_REGISTER.  The second parameter is ignored;
 *        CmdId is always resolved from the class name.
 */
#define REGISTERCMD(className, id) CMD_REGISTER(className)

/**
 * @def REGISTEROUTPUT(className)
 * @brief Register an OutputNode subclass.
 */
#define REGISTEROUTPUT(className) \
    static zrcsSystem::registerAndAddOutputNode<class className> register_##className;

/**
 * @def REGISTERINPUT(className)
 * @brief Register an InputNode subclass.
 */
#define REGISTERINPUT(className) \
    static zrcsSystem::registerAndAddInputNode<class className> register_##className;

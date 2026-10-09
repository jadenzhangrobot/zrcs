/**
 * @file NodeFactory.h
 * @brief Node factory and static-registration helpers.
 *
 * Command and periodic nodes are registered via macros (REGISTERCMD,
 * REGISTER_PERIODIC, with thin REGISTERINPUT/REGISTEROUTPUT aliases) that
 * write shared_ptr instances into function-local static pending lists.  At
 * construction, the NodeFactory drains those lists into its internal
 * registries.
 *
 * Lookup is O(1): command nodes are indexed directly by CmdId enum value.
 *
 * Ordering: periodic nodes keep two containers - inputPeriodics
 * (NodePhase::INPUT, run before commands) and outputPeriodics
 * (NodePhase::OUTPUT, run after commands).  Each is stable-sorted by
 * execOrder_ once, before the RT task starts, via sortPeriodics().
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
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
    using Periodic = std::shared_ptr<PeriodicNode>;

    /// Registered periodic nodes with Phase::OUTPUT (executed in the output
    /// phase, after commands), in execOrder_ ascending order.
    std::vector<Periodic> outputPeriodics;
    /// Registered periodic nodes with Phase::INPUT (executed in the input
    /// phase, before commands), in execOrder_ ascending order.
    std::vector<Periodic>  inputPeriodics;

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

    /// stable-sort inputPeriodics and outputPeriodics by execOrder_ ascending.
    /// Call once before the RT task starts (not on the RT hot path).
    void sortPeriodics();

    /// Check whether a command with the given @p name has been registered.
    bool exist(std::string_view name) const;

    // ---- Static pending lists (populated by registration macros) ------------
    // Function-local statics avoid static-initialisation-order issues.

    /// Pending registration by (name, creator) pair.
    struct PendingCmd    { std::string_view name; Creator creator; };
    /// Pending registration by (CmdId, creator) pair -- preferred path.
    struct PendingCmdById { CmdId cmdId; Creator creator; };
    /// Pending periodic-node registration for the output phase.
    struct PendingOutput { Periodic node; };
    /// Pending periodic-node registration for the input phase.
    struct PendingInput  { Periodic node; };

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
 * @brief Register a PeriodicNode subclass with an explicit phase and order.
 * @tparam T PeriodicNode subclass to register.
 */
template <typename T>
class RegisterPeriodic {
public:
    explicit RegisterPeriodic(NodePhase phase, std::uint32_t order)
    {
        auto node = std::make_shared<T>();
        node->phase_     = phase;
        node->execOrder_ = order;
        if (phase == NodePhase::INPUT) {
            NodeFactory::pendingInputs().push_back({std::move(node)});
        } else {
            NodeFactory::pendingOutputs().push_back({std::move(node)});
        }
    }
};

} // namespace zrcsSystem

// ===========================================================================
// Registration macros
// ===========================================================================

/**
 * @def REGISTERCMD(className)
 * @brief Register a CmdNode subclass using the same-named CmdId enumerator.
 *
 * The class and CmdId naming convention is checked by the compiler: for
 * example, REGISTERCMD(MoveV) binds MoveV to CmdId::MoveV.
 */
#define REGISTERCMD(className) \
    static zrcsSystem::RegisterNodeById<class className> register##className( \
        CmdId::className);

/**
 * @def REGISTER_PERIODIC(className, phase, order)
 * @brief Register a PeriodicNode subclass with an explicit phase and within-
 *        phase execution order (ascending; smaller runs first).
 *
 * Examples:
 *   REGISTER_PERIODIC(DataPub,        INPUT,  10);
 *   REGISTER_PERIODIC(PlcLogicNode,   OUTPUT, 100);
 */
#define REGISTER_PERIODIC(className, phase, order)                        \
    static ::zrcsSystem::RegisterPeriodic<class className> reg_##className( \
        ::zrcsSystem::NodePhase::phase,                                     \
        static_cast< ::std::uint32_t>(order))

/**
 * @def REGISTERINPUT(className)
 * @brief Register a PeriodicNode subclass in the input phase (default order).
 *        Thin alias kept for gradual migration; prefer REGISTER_PERIODIC.
 */
#define REGISTERINPUT(className) \
    REGISTER_PERIODIC(className, INPUT, 10)

/**
 * @def REGISTEROUTPUT(className)
 * @brief Register a PeriodicNode subclass in the output phase (default order).
 *        Thin alias kept for gradual migration; prefer REGISTER_PERIODIC.
 */
#define REGISTEROUTPUT(className) \
    REGISTER_PERIODIC(className, OUTPUT, 10)

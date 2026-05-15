#pragma once

#include <cstddef>
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <string_view>

enum CmdId      {
    INVALID      = 0,
    Enable       = 1,
    Disable      = 2,
    Reset        = 3,
    Setmode      = 4,
    SetZero      = 5,
    JogabsJ      = 6,
    JogJ         = 7,
    MoveAbs      = 8,
    MoveAbsJ     = 9,
    MoveJ        = 10,
    MoveL        = 11,
    MoveC        = 12,
    Movehome     = 13,
    MoveLGalvo   = 14,
    SENTINEL     = 15
};

enum class EnableArg : std::size_t { AxisId = 0 };
enum class DisableArg : std::size_t { AxisId = 0 };
enum class ResetArg : std::size_t { AxisId = 0 };
enum class SetmodeArg : std::size_t { AxisId = 0, Mode };
enum class SetZeroArg : std::size_t { AxisId = 0 };
enum class JogabsJArg : std::size_t { AxisId = 0, TargetPosition };
enum class JogJArg : std::size_t { AxisId = 0, TargetPosition };
enum class MoveAbsArg : std::size_t { AxisId = 0, Position, Vel, Acc, Jerk };
enum class MoveAbsJArg : std::size_t { Count = 0, J1, J2, J3, J4, J5, J6 };
enum class MoveJArg : std::size_t { X = 0, Y, Z, RX, RY, RZ, Vel };
enum class MoveLArg : std::size_t {CurrentX = 0, CurrentY, CurrentZ, CurrentQ1, CurrentQ2, CurrentQ3, CurrentQ4, X, Y, Z, Q1, Q2, Q3, Q4, Vel, CurrentVel, CurrentAcc, TargetVel, TargetAcc};
enum class MoveCArg : std::size_t { ViaX = 0, ViaY, ViaZ, EndX, EndY, EndZ, Vel };
enum class MoveLGalvoArg : std::size_t {CurrentX = 0, CurrentY, CurrentZ, CurrentQ1, CurrentQ2, CurrentQ3, CurrentQ4, X, Y, Z, Q1, Q2, Q3, Q4, Vel, CurrentVel, CurrentAcc, TargetVel, TargetAcc};



inline const char* cmdIdToName(CmdId id) noexcept 
{
    const auto name = magic_enum::enum_name(id);
    return name.empty() ? "UNKNOWN" : name.data();
}

inline std::optional<CmdId> cmdNameToId(std::string_view name) noexcept
{
    return magic_enum::enum_cast<CmdId>(name);
}

namespace zrcs {

using ::CmdId;

inline const char* cmdIdToName(CmdId id) noexcept
{
    return ::cmdIdToName(id);
}

inline const char* cmdIdToName(std::uint16_t id) noexcept
{
    return ::cmdIdToName(static_cast<CmdId>(id));
}

inline std::optional<CmdId> cmdNameToId(std::string_view name) noexcept
{
    return ::cmdNameToId(name);
}

} // namespace zrcs

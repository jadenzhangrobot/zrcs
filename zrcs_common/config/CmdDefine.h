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
    Stop         = 4,
    EmergStop    = 5,
    Setmode      = 6,
    SetOverride  = 7,
    ActUnit      = 8,
    DeactUnit    = 9,
    SetZero      = 10,
    SetDO        = 11,
    SetGO        = 12,
    SetAO        = 13,
    PulseDO      = 14,
    IORead       = 15,
    Wait         = 16,
    WaitDI       = 17,
    WaitUntil    = 18,
    JogabsJ      = 19,
    JogJ         = 20,
    MoveAbs      = 21,
    MoveRel      = 22,
    MoveAbsJ     = 23,
    MoveJ        = 24,
    MoveL        = 25,
    MoveC        = 26,
    SearchL      = 27,
    TriggJ       = 28,
    TriggL       = 29,
    Movehome     = 30,
    SetTCP       = 31,
    SetBase      = 32,
    SetPayload   = 33,
    ConfJ        = 34,
    ConfL        = 35,
    SingArea     = 36,
    SetPosLimit  = 37,
    SetVelLimit  = 38,
    GetFK        = 39,
    GetJointPos  = 40,
    SyncMove     = 41,
    CamMove      = 42,
    Probe        = 43,
    PosCapture   = 44,
    PosCompare   = 45,
    HelixMove    = 46,
    BufMove      = 47,
    PathMove     = 48,
    LaserSet     = 49,
    GalvoMarkL   = 50,
    GalvoBufMark = 51,
    MoveLGalvo   = 52,
    SENTINEL     = 53
};

enum class EnableArg : std::size_t { AxisId = 0 };
enum class DisableArg : std::size_t { AxisId = 0 };
enum class ResetArg : std::size_t { AxisId = 0 };
enum class SetmodeArg : std::size_t { AxisId = 0, Mode };
enum class SetZeroArg : std::size_t { AxisId = 0 };
enum class SetDOArg : std::size_t { ModuleIndex = 0, BitPos, Value };
enum class SetAOArg : std::size_t { ModuleIndex = 0, Channel, Value };
enum class JogabsJArg : std::size_t { AxisId = 0, TargetPosition };
enum class JogJArg : std::size_t { AxisId = 0, TargetPosition };
enum class MoveAbsArg : std::size_t { AxisId = 0, Position, Vel, Acc, Jerk };
enum class MoveRelArg : std::size_t { AxisId = 0, Distance, Vel, Acc, Jerk };
enum class MoveAbsJArg : std::size_t { Count = 0, J1, J2, J3, J4, J5, J6 };
enum class MoveJArg : std::size_t { X = 0, Y, Z, RX, RY, RZ, Vel };
enum class MoveLArg : std::size_t {CurrentX = 0, CurrentY, CurrentZ, CurrentRX, CurrentRY, CurrentRZ,X, Y, Z, RX, RY, RZ, Vel, CurrentVel, CurrentAcc, TargetVel, TargetAcc};
enum class MoveCArg : std::size_t { ViaX = 0, ViaY, ViaZ, EndX, EndY, EndZ, Vel };
enum class SetBaseArg : std::size_t { X = 0, Y, Z, RX, RY, RZ };
enum class MoveLGalvoArg : std::size_t {CurrentX = 0, CurrentY, CurrentZ, CurrentRX, CurrentRY, CurrentRZ,X, Y, Z, RX, RY, RZ, Vel, CurrentVel, CurrentAcc, TargetVel, TargetAcc};



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

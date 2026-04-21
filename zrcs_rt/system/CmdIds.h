#pragma once
/*
 * CmdIds.h — 手动维护的命令 ID 枚举、名称查表和名称→ID 映射
 *
 * 替代原先由 tool/gen_cmd_registry.py 生成的三个文件：
 *   CmdRegistry_gen.h / CmdHead_gen.h / CmdNameToId_gen.h
 *
 * 新增命令时：
 *   1. 在 enum class CmdId 中添加枚举项（注意保持 SENTINEL = 最大值+1）
 *   2. 在 cmdIdToName() 的 kTable 数组中添加对应名称
 *   3. 在 cmdNameToId() 的 kMap 中添加对应映射
 *   4. 在 command/CmdHead.h 中添加 #include
 */

#include <cstdint>
#include <string>
#include <unordered_map>
#include "shared_memory/ShmLayout.h"

enum class CmdId : uint16_t {
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

inline constexpr size_t AXISMAXCOUNT = zrcs::kAxisMax;

namespace zrcs {

inline const char* cmdIdToName(uint16_t id) noexcept {
    static constexpr const char* kTable[] = {
        "INVALID",        // 0
        "Enable",         // 1
        "Disable",        // 2
        "Reset",          // 3
        "Stop",           // 4
        "EmergStop",      // 5
        "Setmode",        // 6
        "SetOverride",    // 7
        "ActUnit",        // 8
        "DeactUnit",      // 9
        "SetZero",        // 10
        "SetDO",          // 11
        "SetGO",          // 12
        "SetAO",          // 13
        "PulseDO",        // 14
        "IORead",         // 15
        "Wait",           // 16
        "WaitDI",         // 17
        "WaitUntil",      // 18
        "JogabsJ",        // 19
        "JogJ",           // 20
        "MoveAbs",        // 21
        "MoveRel",        // 22
        "MoveAbsJ",       // 23
        "MoveJ",          // 24
        "MoveL",          // 25
        "MoveC",          // 26
        "SearchL",        // 27
        "TriggJ",         // 28
        "TriggL",         // 29
        "Movehome",       // 30
        "SetTCP",         // 31
        "SetBase",        // 32
        "SetPayload",     // 33
        "ConfJ",          // 34
        "ConfL",          // 35
        "SingArea",       // 36
        "SetPosLimit",    // 37
        "SetVelLimit",    // 38
        "GetFK",          // 39
        "GetJointPos",    // 40
        "SyncMove",       // 41
        "CamMove",        // 42
        "Probe",          // 43
        "PosCapture",     // 44
        "PosCompare",     // 45
        "HelixMove",      // 46
        "BufMove",        // 47
        "PathMove",       // 48
        "LaserSet",       // 49
        "GalvoMarkL",     // 50
        "GalvoBufMark",   // 51
        "MoveLGalvo",     // 52
    };
    constexpr size_t kTableSize = sizeof(kTable) / sizeof(kTable[0]);
    if (id < kTableSize) return kTable[id];
    return "UNKNOWN";
}

inline const std::unordered_map<std::string, CmdId>& cmdNameToId() {
    static const std::unordered_map<std::string, CmdId> kMap = {
        {"Enable",        CmdId::Enable},
        {"Disable",       CmdId::Disable},
        {"Reset",         CmdId::Reset},
        {"Stop",          CmdId::Stop},
        {"EmergStop",     CmdId::EmergStop},
        {"Setmode",       CmdId::Setmode},
        {"SetOverride",   CmdId::SetOverride},
        {"ActUnit",       CmdId::ActUnit},
        {"DeactUnit",     CmdId::DeactUnit},
        {"SetZero",       CmdId::SetZero},
        {"SetDO",         CmdId::SetDO},
        {"SetGO",         CmdId::SetGO},
        {"SetAO",         CmdId::SetAO},
        {"PulseDO",       CmdId::PulseDO},
        {"IORead",        CmdId::IORead},
        {"Wait",          CmdId::Wait},
        {"WaitDI",        CmdId::WaitDI},
        {"WaitUntil",     CmdId::WaitUntil},
        {"JogabsJ",       CmdId::JogabsJ},
        {"JogJ",          CmdId::JogJ},
        {"MoveAbs",       CmdId::MoveAbs},
        {"MoveRel",       CmdId::MoveRel},
        {"MoveAbsJ",      CmdId::MoveAbsJ},
        {"MoveJ",         CmdId::MoveJ},
        {"MoveL",         CmdId::MoveL},
        {"MoveC",         CmdId::MoveC},
        {"SearchL",       CmdId::SearchL},
        {"TriggJ",        CmdId::TriggJ},
        {"TriggL",        CmdId::TriggL},
        {"Movehome",      CmdId::Movehome},
        {"SetTCP",        CmdId::SetTCP},
        {"SetBase",       CmdId::SetBase},
        {"SetPayload",    CmdId::SetPayload},
        {"ConfJ",         CmdId::ConfJ},
        {"ConfL",         CmdId::ConfL},
        {"SingArea",      CmdId::SingArea},
        {"SetPosLimit",   CmdId::SetPosLimit},
        {"SetVelLimit",   CmdId::SetVelLimit},
        {"GetFK",         CmdId::GetFK},
        {"GetJointPos",   CmdId::GetJointPos},
        {"SyncMove",      CmdId::SyncMove},
        {"CamMove",       CmdId::CamMove},
        {"Probe",         CmdId::Probe},
        {"PosCapture",    CmdId::PosCapture},
        {"PosCompare",    CmdId::PosCompare},
        {"HelixMove",     CmdId::HelixMove},
        {"BufMove",       CmdId::BufMove},
        {"PathMove",      CmdId::PathMove},
        {"LaserSet",      CmdId::LaserSet},
        {"GalvoMarkL",    CmdId::GalvoMarkL},
        {"GalvoBufMark",  CmdId::GalvoBufMark},
        {"MoveLGalvo",    CmdId::MoveLGalvo},
    };
    return kMap;
}

} // namespace zrcs

#pragma once

#include "ConfigSerializer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace zrcs::config {

/**
 * @brief ethercat.xml 中单个 PDO 条目的可序列化数据。
 *
 * 旧属性风格 XML 的 idx/subIdx 是十六进制、寄存器名通过 "index+subindex" 的
 * 字符串拼接进 IndexAndRegister。迁移到 cereal 后全部以十进制落盘，寄存器名
 * 作为 name 字段直接保存在条目上，运行时再从 name 重建查找表。
 */
struct EthercatPdoEntryData {
    uint16_t index = 0;     // 旧 XML 属性 idx（hex）→ 十进制
    uint16_t subIndex = 0;  // 旧 XML 属性 subIdx（hex）→ 十进制
    uint8_t bitLength = 0;  // 旧 XML 属性 bitLen（十进制）
    std::string name;       // 寄存器名，如 "ControlWord"/"TargetPosition"

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(index), CEREAL_NVP(subIndex), CEREAL_NVP(bitLength),
           CEREAL_NVP(name));
    }
};

/// ethercat.xml 中单个 PDO（映射集合）的可序列化数据。
struct EthercatPdoData {
    uint16_t index = 0;  // 旧 XML 属性 idx（hex）→ 十进制，如 0x1600 → 5632
    std::vector<EthercatPdoEntryData> entries;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(index), CEREAL_NVP(entries));
    }
};

/// ethercat.xml 中单个 SyncManager 的可序列化数据。
struct EthercatSyncManagerData {
    uint16_t index = 0;   // 旧 XML 属性 idx（十进制，0..3）
    int direction = 0;    // 旧 XML 属性 dir（1=输出/RxPDO，2=输入/TxPDO）
    uint8_t watchdog = 0; // 旧 XML 属性 watchDog
    std::vector<EthercatPdoData> pdos;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(index), CEREAL_NVP(direction), CEREAL_NVP(watchdog),
           CEREAL_NVP(pdos));
    }
};

/// ethercat.xml 中单个从站的可序列化数据。
struct EthercatSlaveData {
    uint16_t slaveId = 0;        // 旧 XML 属性 ID（十进制）
    std::string type = "motor";  // motor / aio / dio / laser
    uint32_t vId = 0;            // 旧 XML 属性 VID（hex）→ 十进制
    uint32_t pId = 0;            // 旧 XML 属性 PID（hex）→ 十进制
    uint16_t assignActivate = 0; // 旧 XML 属性 assignActivate（hex）→ 十进制
    uint32_t sync0Cycle = 0;     // 旧 XML 属性 sync0Cycle（十进制）
    uint32_t sync0Shift = 0;     // 旧 XML 属性 sync0Shift（十进制）
    std::vector<EthercatSyncManagerData> syncManagers;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(slaveId), CEREAL_NVP(type), CEREAL_NVP(vId),
           CEREAL_NVP(pId), CEREAL_NVP(assignActivate),
           CEREAL_NVP(sync0Cycle), CEREAL_NVP(sync0Shift), CEREAL_NVP(syncManagers));
    }
};

/// ethercat.xml 的顶层数据对象。从站与 SyncManager 的顺序是加载相关的
/// （EthercatMaster 按位置注册并索引 SMOUT/SMIN），cereal 的 vector 序列化
/// 保留顺序。
struct EthercatConfigFile {
    std::vector<EthercatSlaveData> slaves;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(slaves));
    }
};

/// ethercat.xml 根节点固定为 "ethercatConfig"，由 ConfigManager::ethercatPath()
/// 指定的文件加载。
template <>
struct XmlRoot<EthercatConfigFile> {
    static constexpr const char* name() { return "ethercatConfig"; }
};

} // namespace zrcs::config

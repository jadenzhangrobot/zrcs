/**
 * @file EthercatParameter.h
 * @brief EtherCAT 从站配置解析器 — 从 ethercat.xml 解析所有从站的 PDO/SyncManager 信息
 *
 * 根据 ENI (EtherCAT Network Information) 格式的 XML 文件，提取每个从站的:
 * - 厂商/产品 ID (VID/PID)
 * - 从站类型 (MOTOR/AIO/DIO/LASER)
 * - SyncManager 配置 (同步管理器方向、看门狗模式)
 * - PDO 映射 (RxPDO/TxPDO 的索引、子索引、位宽)
 * - DC (Distributed Clocks) 同步参数
 *
 * XML 解析统一走 zrcs_common/config 的 cereal 入口 (EthercatConfigFile)，
 * 本类只负责把解析出的可序列化数据转换成 IgH 需要的 ec_* 运行时结构。
 * 配置文件里的数值全部以十进制落盘；PDO 索引等在旧属性风格里是十六进制的，
 * 迁移时已换算（详见 zrcs_common/config/EthercatConfig.h）。
 *
 * @author zhangyongjing
 */
#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "ecrt.h"
#include "config/EthercatConfig.h"

namespace ZrcsHardware {

/**
 * @brief EtherCAT 从站配置解析器
 *
 * 解析 ethercat.xml 文件，构建所有从站的 SyncManager 和 PDO 映射信息。
 * 默认文件名为 "ethercat.xml"，可通过构造函数参数指定完整路径。
 */
class SlaveConfig {
public:
    /// EtherCAT 从站类型枚举
    enum SlaveType {
        MOTOR,  ///< 伺服电机驱动器
        AIO,    ///< 模拟量 IO 模块
        DIO,    ///< 数字量 IO 模块
        LASER,  ///< 激光器模块
    };

    /// 单个从站的完整配置信息
    using Slave = struct {
        uint16_t SlaveId;                           ///< 从站 ID
        SlaveType slaveType;                        ///< 从站类型
        uint32_t VID;                               ///< 厂商 ID
        uint32_t PID;                               ///< 产品 ID
        uint16_t AssignActivate;                    ///< DC AssignActivate 寄存器值
        uint32_t Sync0Cycle;                        ///< Sync0 周期时间 (ns)
        uint32_t Sync0Shift;                        ///< Sync0 偏移时间 (ns)
        std::vector<ec_sync_info_t> EcSms;          ///< SyncManager 配置列表

        /// 索引+子索引 -> 寄存器名称映射，供 EthercatMaster 按名称解析偏移。
        std::map<std::string, std::string> IndexAndRegister;

        // EcSms 里的指针指向下面的稳定存储。分两档：OwnedPdos 每个
        // SyncManager 一个 vector，OwnedEntries 每个 PDO 一个 vector。
        // 用成员 vector 持有数据，析构时自动释放（旧实现用裸指针只会删最后一个）。
        std::vector<std::vector<ec_pdo_info_t>> OwnedPdos;
        std::vector<std::vector<ec_pdo_entry_info_t>> OwnedEntries;
    };

    std::vector<Slave> Slaves;  ///< 所有从站的配置信息

    /**
     * @brief 构造函数 — 加载并转换 ethercat.xml 的从站配置
     * @param ethercatFile ethercat.xml 的文件路径
     * @throws std::runtime_error 文件缺失、XML 无效或从站类型未知时抛出
     */
    SlaveConfig(const std::string& ethercatFile = "ethercat.xml")
    {
        const auto file = zrcs::config::loadXml<zrcs::config::EthercatConfigFile>(ethercatFile);

        for (const auto& sd : file.slaves) {
            Slave slave;
            slave.SlaveId = sd.slaveId;
            slave.slaveType = parseSlaveType(sd.type);
            slave.VID = sd.vId;
            slave.PID = sd.pId;
            slave.AssignActivate = sd.assignActivate;
            slave.Sync0Cycle = sd.sync0Cycle;
            slave.Sync0Shift = sd.sync0Shift;

            // 先按数量预留外层容量，保证填充过程中内部 vector 不被移动，
            // EcSms 里保存的 data() 指针稳定有效。
            slave.OwnedPdos.reserve(sd.syncManagers.size());
            size_t totalPdos = 0;
            for (const auto& sm : sd.syncManagers) {
                totalPdos += sm.pdos.size();
            }
            slave.OwnedEntries.reserve(totalPdos);

            for (const auto& sm : sd.syncManagers) {
                ec_sync_info_t ecsm;
                ecsm.index = sm.index;
                ecsm.dir = static_cast<ec_direction_t>(sm.direction);
                ecsm.watchdog_mode = static_cast<ec_watchdog_mode_t>(sm.watchdog);

                slave.OwnedPdos.emplace_back();
                auto& pdoList = slave.OwnedPdos.back();
                pdoList.reserve(sm.pdos.size());

                for (const auto& pd : sm.pdos) {
                    slave.OwnedEntries.emplace_back();
                    auto& entryList = slave.OwnedEntries.back();
                    entryList.reserve(pd.entries.size());
                    for (const auto& en : pd.entries) {
                        ec_pdo_entry_info_t entry;
                        entry.index = en.index;
                        entry.subindex = en.subIndex;
                        entry.bit_length = en.bitLength;
                        slave.IndexAndRegister.insert(std::make_pair(
                            std::to_string(en.index) + std::to_string(en.subIndex),
                            en.name));
                        entryList.push_back(entry);
                    }

                    ec_pdo_info_t pdo;
                    pdo.index = pd.index;
                    pdo.n_entries = static_cast<uint8_t>(entryList.size());
                    pdo.entries = entryList.data();
                    pdoList.push_back(pdo);
                }

                ecsm.n_pdos = static_cast<uint8_t>(pdoList.size());
                ecsm.pdos = pdoList.empty() ? nullptr : pdoList.data();
                slave.EcSms.push_back(ecsm);
            }

            // IgH 用 index=0xff 的哨兵结束 PDO 配置列表（不落盘，运行时生成）。
            ec_sync_info_t sentinel{};
            sentinel.index = 0xff;
            slave.EcSms.push_back(sentinel);

            Slaves.push_back(std::move(slave));
        }
    }

private:
    static SlaveType parseSlaveType(const std::string& type)
    {
        if (type == "motor") {
            return SlaveType::MOTOR;
        }
        if (type == "aio") {
            return SlaveType::AIO;
        }
        if (type == "dio") {
            return SlaveType::DIO;
        }
        if (type == "laser") {
            return SlaveType::LASER;
        }
        throw std::runtime_error("Add the correct slave type");
    }
};

} // namespace ZrcsHardware

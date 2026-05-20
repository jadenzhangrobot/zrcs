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
 * SlaveConfig 结构体中的成员命名保留了 XML 属性的大写首字母风格，
 * 因为它们与 ethercat.xml 中的属性名称一一对应，便于维护对照。
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
#include "tinyxml2.h"

#include "config/Parameter.h"

namespace ZrcsHardware {

/**
 * @brief EtherCAT 从站配置解析器
 *
 * 解析 ethercat.xml 文件，构建所有从站的 SyncManager 和 PDO 映射信息。
 * 默认文件名为 "ethercat.xml"，可通过构造函数参数指定完整路径。
 *
 * XML 结构示例:
 * @code{.xml}
 * <slave ID="0" type="motor" VID="0x..." PID="0x..." configPdos="true"
 *        assignActivate="0x0300" sync0Cycle="1000000" sync0Shift="0">
 *   <syncManager idx="2" dir="1" watchDog="0">
 *     <pdo idx="0x1600">
 *       <entry idx="0x6040" subIdx="0x00" bitLen="16" name="ControlWord"/>
 *       <entry idx="0x607A" subIdx="0x00" bitLen="32" name="TargetPosition"/>
 *     </pdo>
 *   </syncManager>
 * </slave>
 * @endcode
 */
class SlaveConfig {
public:
    std::vector<ec_pdo_entry_info_t>* entries;  ///< PDO 条目列表（动态分配）
    std::vector<ec_pdo_info_t>* pdos;           ///< PDO 列表（动态分配）

    /// EtherCAT 从站类型枚举
    enum SlaveType {
        MOTOR,  ///< 伺服电机驱动器
        AIO,    ///< 模拟量 IO 模块
        DIO,    ///< 数字量 IO 模块
        LASER,  ///< 激光器模块
    };

    /// 单个从站的完整配置信息
    using Slave = struct {
        std::string SlaveName;                      ///< 从站名称
        uint16_t SlaveId;                           ///< 从站 ID
        SlaveType slaveType;                        ///< 从站类型
        uint32_t VID;                               ///< 厂商 ID (hex)
        uint32_t PID;                               ///< 产品 ID (hex)
        bool ConfigPdo;                             ///< 是否需要配置 PDO
        uint16_t AssignActivate;                    ///< DC AssignActivate 寄存器值
        uint32_t Sync0Cycle;                        ///< Sync0 周期时间 (ns)
        uint32_t Sync0Shift;                        ///< Sync0 偏移时间 (ns)
        std::vector<ec_sync_info_t> EcSms;          ///< SyncManager 配置列表
        std::map<std::string, std::string> IndexAndRegister;  ///< 索引+子索引 -> 寄存器名称映射
    };

    std::vector<Slave> Slaves;  ///< 所有从站的配置信息

    /**
     * @brief 构造函数 — 从 XML 文件加载所有从站配置
     * @param ethercatFile ethercat.xml 的文件路径
     * @throws std::runtime_error 从站类型未知时抛出
     */
    SlaveConfig(const std::string& ethercatFile = "ethercat.xml")
    {
        tinyxml2::XMLDocument doc;
        const auto status = doc.LoadFile(ethercatFile.c_str());
        if (status != tinyxml2::XML_SUCCESS) {
            throw std::runtime_error("Failed to read EtherCAT XML: " + ethercatFile);
        }
        auto* root = doc.RootElement();
        if (root == nullptr) {
            throw std::runtime_error("EtherCAT XML has no root element: " + ethercatFile);
        }

        for (auto* slaveNode = root->FirstChildElement(); slaveNode;
             slaveNode = slaveNode->NextSiblingElement()) {
            Slave slave;
            slave.SlaveId = std::stoi(slaveNode->Attribute("ID"));
            slave.VID = std::stoll(slaveNode->Attribute("VID"), nullptr, 16);
            slave.PID = std::stoll(slaveNode->Attribute("PID"), nullptr, 16);
            slave.ConfigPdo = std::string(slaveNode->Attribute("configPdos")) == "true";

            const std::string type = slaveNode->Attribute("type");
            if (type == "motor") {
                slave.slaveType = SlaveType::MOTOR;
            } else if (type == "aio") {
                slave.slaveType = SlaveType::AIO;
            } else if (type == "dio") {
                slave.slaveType = SlaveType::DIO;
            } else if (type == "laser") {
                slave.slaveType = SlaveType::LASER;
            } else {
                throw std::runtime_error("Add the correct slave type");
            }

            slave.AssignActivate = std::stoi(slaveNode->Attribute("assignActivate"), nullptr, 16);
            slave.Sync0Cycle = std::stoi(slaveNode->Attribute("sync0Cycle"));
            slave.Sync0Shift = std::stoi(slaveNode->Attribute("sync0Shift"));

            for (auto* syncNode = slaveNode->FirstChildElement(); syncNode;
                 syncNode = syncNode->NextSiblingElement()) {
                ec_sync_info_t ecsm;
                ecsm.index = std::stoi(syncNode->Attribute("idx"));
                ecsm.dir = static_cast<ec_direction_t>(std::stoi(syncNode->Attribute("dir")));
                ecsm.watchdog_mode = static_cast<ec_watchdog_mode_t>(
                    std::stoi(syncNode->Attribute("watchDog")));

                pdos = new std::vector<ec_pdo_info_t>;
                for (auto* pdoNode = syncNode->FirstChildElement(); pdoNode;
                     pdoNode = pdoNode->NextSiblingElement()) {
                    ec_pdo_info_t pdo_;
                    pdo_.index = std::stoi(pdoNode->Attribute("idx"), nullptr, 16);

                    entries = new std::vector<ec_pdo_entry_info_t>;
                    for (auto* entryNode = pdoNode->FirstChildElement(); entryNode;
                         entryNode = entryNode->NextSiblingElement()) {
                        ec_pdo_entry_info_t pdoEntry_;
                        pdoEntry_.index = std::stoi(entryNode->Attribute("idx"), nullptr, 16);
                        pdoEntry_.subindex = std::stoi(entryNode->Attribute("subIdx"), nullptr, 16);
                        pdoEntry_.bit_length = std::stoi(entryNode->Attribute("bitLen"));
                        slave.IndexAndRegister.insert(std::make_pair(
                            std::to_string(pdoEntry_.index) + std::to_string(pdoEntry_.subindex),
                            entryNode->Attribute("name")));
                        entries->push_back(pdoEntry_);
                    }
                    pdo_.n_entries = entries->size();
                    pdo_.entries = entries->data();
                    pdos->push_back(pdo_);
                }
                ecsm.n_pdos = pdos->size();
                if (pdos->size() == 0) {
                    ecsm.pdos = nullptr;
                } else {
                    ecsm.pdos = pdos->data();
                }
                slave.EcSms.push_back(ecsm);
            }
            ec_sync_info_t esit;
            esit.index = 0xff;
            slave.EcSms.push_back(esit);
            Slaves.push_back(slave);
        }
    }

    ~SlaveConfig()
    {
        delete entries;
        delete pdos;
    }
};

} // namespace ZrcsHardware

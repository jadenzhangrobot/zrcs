/**
 * @file EthercatMaster.h
 * @brief EtherCAT 主站 — 基于 IgH EtherCAT Master 库的实时现场总线封装
 *
 * 职责:
 * - 请求并激活 EtherCAT 主站
 * - 创建 Process Data Domain (Input/Output)
 * - 配置所有从站的 SyncManager 和 PDO 映射
 * - 配置 Distributed Clocks (DC) 时钟同步
 * - 管理 PDO 偏移表和寄存器名称映射
 * - 提供 RT 安全的 send()/receive() 周期接口
 *
 * 依赖: IgH EtherCAT Master userspace library (libethercat)
 *
 * @author zhangyongjing
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#ifdef REALTIME
#include <alchemy/timer.h>
#include "ecrt.h"
#endif

#include <time.h>

#include "controller/HardwareBus.h"
#include "EthercatParameter.h"

namespace ZrcsHardware {

#define SMOUT 2  ///< EtherCAT SyncManager 2 索引 (通常为输出/RxPDO)
#define SMIN 3   ///< EtherCAT SyncManager 3 索引 (通常为输入/TxPDO)

/**
 * @brief EtherCAT 主站
 *
 * 管理整个 EtherCAT 总线，包括从站配置、PDO 映射、DC 时钟同步和
 * 周期性数据收发。所有静态成员（master、domain、PDO 寄存器等）定义为
 * static inline 以支持在实时线程上下文中使用。
 *
 * 生命周期:
 *   1. 构造 -> EthercatInit() 完成总线初始化和激活
 *   2. RT 循环中调用 send() / receive()
 *   3. 析构 -> ecrt_release_master() 释放主站
 */
class EthercatMaster : public HardwareBus {
private:
    static inline ec_master_t* master_ = NULL;                  ///< EtherCAT 主站句柄
    static inline ec_master_state_t master_state_ = {};          ///< 主站状态

    static inline ec_domain_t* domain_input_ = NULL;             ///< 输入 Domain (TxPDO)
    static inline ec_domain_t* domain_output_ = NULL;            ///< 输出 Domain (RxPDO)
    static inline ec_domain_state_t domain_state_ = {};          ///< Domain 状态

    static inline ec_slave_config_t* sc_;                        ///< 从站配置句柄
    static inline ec_slave_config_state_t sc_state_ = {};        ///< 从站配置状态

    static inline std::vector<ec_pdo_entry_reg_t> domain_input_reg_;   ///< 输入 PDO 寄存器列表
    static inline std::vector<ec_pdo_entry_reg_t> domain_output_reg_;  ///< 输出 PDO 寄存器列表

    int all_output_pdo_count_ = 0;   ///< 所有从站的输出 PDO 条目总数
    int all_input_pdo_count_ = 0;    ///< 所有从站的输入 PDO 条目总数

public:
    SlaveConfig* slaveConfig;   ///< 从站配置解析器

    static inline uint8_t* DomainWrite = NULL;                          ///< 输出 Domain 数据指针
    static inline uint8_t* DomainRead = NULL;                           ///< 输入 Domain 数据指针
    static inline std::vector<std::vector<uint32_t>> OutputOffset;      ///< 每个从站的输出 PDO 偏移表
    static inline std::vector<std::vector<uint32_t>> InputOffset;       ///< 每个从站的输入 PDO 偏移表

    std::vector<std::map<std::string, int>> InputPdoInfoAndOffset;     ///< 输入 PDO 寄存器名 -> 偏移映射
    std::vector<std::map<std::string, int>> OutputPdoInfoAndOffset;    ///< 输出 PDO 寄存器名 -> 偏移映射

    /**
     * @brief 构造函数 — 初始化 EtherCAT 主站
     * @param ethercatFile ethercat.xml 的路径，默认为 "ethercat.xml"
     * @throws std::runtime_error 主站初始化、Domain 创建、从站配置或激活失败时抛出
     */
    EthercatMaster(const std::string& ethercatFile = "ethercat.xml")
        : slaveConfig(new SlaveConfig(ethercatFile))
    {
        EthercatInit();
    }

    ~EthercatMaster()
    {
        ecrt_release_master(master_);
        delete slaveConfig;
    }

    /**
     * @brief 完成 EtherCAT 主站的完整初始化流程
     *
     * 流程:
     * 1. 计算所有从站的 PDO 总数，分配偏移表
     * 2. ecrt_request_master() — 请求主站
     * 3. ecrt_master_create_domain() — 创建输入/输出 Domain
     * 4. 为每个从站调用 ecrt_master_slave_config()
     * 5. 配置 DC (Distributed Clocks) 参考时钟和同步参数
     * 6. 注册所有 PDO 条目到 Domain
     * 7. ecrt_master_activate() — 激活主站
     * 8. 获取 Domain 数据指针
     *
     * @throws std::runtime_error 任意步骤失败时抛出
     */
    void EthercatInit()
    {
        OutputOffset.resize(slaveConfig->Slaves.size());
        InputOffset.resize(slaveConfig->Slaves.size());
        InputPdoInfoAndOffset.resize(slaveConfig->Slaves.size());
        OutputPdoInfoAndOffset.resize(slaveConfig->Slaves.size());

        for (int i = 0; i < slaveConfig->Slaves.size(); i++) {
            int OutputPdoCount = 0;
            int InputPdoCount = 0;

            for (int j = 0; j < slaveConfig->Slaves[i].EcSms[SMOUT].n_pdos; j++) {
                for (int k = 0; k < slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries; k++) {
                    OutputPdoCount++;
                }
            }

            for (int j = 0; j < slaveConfig->Slaves[i].EcSms[SMIN].n_pdos; j++) {
                for (int k = 0; k < slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries; k++) {
                    InputPdoCount++;
                }
            }
            all_output_pdo_count_ = all_output_pdo_count_ + OutputPdoCount;
            all_input_pdo_count_ = all_input_pdo_count_ + InputPdoCount;
            OutputOffset[i].resize(OutputPdoCount);
            InputOffset[i].resize(InputPdoCount);
        }
        // IgH expects each PDO registration list to end with a zeroed entry.
        // assign() also clears stale data if a master is created again.
        domain_output_reg_.assign(all_output_pdo_count_ + 1, {});
        domain_input_reg_.assign(all_input_pdo_count_ + 1, {});

        master_ = ecrt_request_master(0);
        if (master_ == nullptr) {
            throw std::runtime_error("Failed to acquire EtherCAT master");
        }

        domain_input_ = ecrt_master_create_domain(master_);
        domain_output_ = ecrt_master_create_domain(master_);
        if ((domain_input_ == nullptr) || (domain_output_ == nullptr)) {
            throw std::runtime_error("Failed to create EtherCAT domain");
        }

        for (int i = 0; i < slaveConfig->Slaves.size(); i++) {
            sc_ = ecrt_master_slave_config(master_, 0, i,
                slaveConfig->Slaves[i].VID, slaveConfig->Slaves[i].PID);
            if (sc_ == NULL) {
                throw std::runtime_error("Failed to configure EtherCAT slave");
            }

            if (ecrt_slave_config_pdos(sc_, EC_END, slaveConfig->Slaves[i].EcSms.data()) < 0) {
                throw std::runtime_error("Failed to configure EtherCAT PDOs");
            }

            if (i == 0) {
#ifdef REALTIME
                if (ecrt_master_select_reference_clock(master_, sc_) < 0) {
                    throw std::runtime_error("Failed to select reference clock");
                }
#endif
            }

            ecrt_slave_config_dc(sc_, slaveConfig->Slaves[i].AssignActivate,
                slaveConfig->Slaves[i].Sync0Cycle, slaveConfig->Slaves[i].Sync0Shift, 0, 0);

            // 注册输出 PDO 条目
            for (int j = 0; j < slaveConfig->Slaves[i].EcSms[SMOUT].n_pdos; j++) {
                for (int k = 0; k < slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries; k++) {
                    uint16_t count = i * slaveConfig->Slaves[i].EcSms[SMOUT].n_pdos *
                                     slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries +
                                     j * slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries + k;
                    uint16_t offset = j * slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries + k;
                    domain_output_reg_[count].alias = 0;
                    domain_output_reg_[count].position = i;
                    domain_output_reg_[count].vendor_id = slaveConfig->Slaves[i].VID;
                    domain_output_reg_[count].product_code = slaveConfig->Slaves[i].PID;
                    domain_output_reg_[count].index =
                        slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].entries[k].index;
                    domain_output_reg_[count].subindex =
                        slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].entries[k].subindex;
                    domain_output_reg_[count].offset = &OutputOffset[i][offset];
                    domain_output_reg_[count].bit_position = nullptr;

                    auto it = slaveConfig->Slaves[i].IndexAndRegister.find(
                        std::to_string(domain_output_reg_[count].index) +
                        std::to_string(domain_output_reg_[count].subindex));
                    if (it != slaveConfig->Slaves[i].IndexAndRegister.end()) {
                        OutputPdoInfoAndOffset[i].emplace(it->second, offset);
                    } else {
                        throw std::runtime_error("Register not found for output PDO entry");
                    }
                }
            }

            // 注册输入 PDO 条目
            for (int j = 0; j < slaveConfig->Slaves[i].EcSms[SMIN].n_pdos; j++) {
                for (int k = 0; k < slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries; k++) {
                    uint16_t count = i * slaveConfig->Slaves[i].EcSms[SMIN].n_pdos *
                                     slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries +
                                     j * slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries + k;
                    uint16_t offset = j * slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries + k;
                    domain_input_reg_[count].alias = 0;
                    domain_input_reg_[count].position = i;
                    domain_input_reg_[count].vendor_id = slaveConfig->Slaves[i].VID;
                    domain_input_reg_[count].product_code = slaveConfig->Slaves[i].PID;
                    domain_input_reg_[count].index =
                        slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].entries[k].index;
                    domain_input_reg_[count].subindex =
                        slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].entries[k].subindex;
                    domain_input_reg_[count].offset = &InputOffset[i][offset];
                    domain_input_reg_[count].bit_position = nullptr;

                    auto it = slaveConfig->Slaves[i].IndexAndRegister.find(
                        std::to_string(domain_input_reg_[count].index) +
                        std::to_string(domain_input_reg_[count].subindex));
                    if (it != slaveConfig->Slaves[i].IndexAndRegister.end()) {
                        InputPdoInfoAndOffset[i].emplace(it->second, offset);
                    } else {
                        throw std::runtime_error("Register not found for input PDO entry");
                    }
                }
            }
        }

        if (ecrt_domain_reg_pdo_entry_list(domain_output_, domain_output_reg_.data()) ||
            ecrt_domain_reg_pdo_entry_list(domain_input_, domain_input_reg_.data())) {
            throw std::runtime_error("Failed to map domain memory");
        }

        if (ecrt_master_activate(master_) < 0) {
            throw std::runtime_error("Failed to activate EtherCAT master");
        }

        if ((DomainWrite = ecrt_domain_data(domain_output_)) == nullptr) {
            throw std::runtime_error("Failed to get domain write address");
        }
        if ((DomainRead = ecrt_domain_data(domain_input_)) == nullptr) {
            throw std::runtime_error("Failed to get domain read address");
        }
    }

    /**
     * @brief 发送过程数据帧
     *
     * RT 安全。在每个控制周期末端调用:
     * 1. ecrt_domain_queue() — 将 Domain 加入发送队列
     * 2. ecrt_master_application_time() — 设置应用时间戳
     * 3. ecrt_master_sync_reference_clock() — 同步参考时钟
     * 4. ecrt_master_sync_slave_clocks() — 同步所有从站时钟
     * 5. ecrt_master_send() — 发送以太网帧
     */
    void send()
    {
        ecrt_domain_queue(domain_output_);
        ecrt_domain_queue(domain_input_);
#ifdef REALTIME
        ecrt_master_application_time(master_, rt_timer_read());
#else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t ns = (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
        ecrt_master_application_time(master_, ns);
#endif
        ecrt_master_sync_reference_clock(master_);
        ecrt_master_sync_slave_clocks(master_);
        ecrt_master_send(master_);
    }

    /**
     * @brief 接收过程数据帧
     *
     * RT 安全。在每个控制周期首端调用:
     * 1. ecrt_master_receive() — 接收入站以太网帧
     * 2. ecrt_domain_process() — 将接收到的数据解包到 Domain 内存
     */
    void receive()
    {
        ecrt_master_receive(master_);
        ecrt_domain_process(domain_output_);
        ecrt_domain_process(domain_input_);
    }
};

} // namespace ZrcsHardware

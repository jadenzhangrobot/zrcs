/**
 * @file Controller.h
 * @brief 控制器聚合类 — 持有所有轴(Axis)、IO、激光器和硬件总线的实例
 *
 * 职责：
 * 1. 每 RT 周期调用 sendData()/receiveData() 完成数据收发
 * 2. sendData(): 对每个轴执行 cmdsProcessing() → updateMotionCmdsToServo() → 总线 send()
 * 3. receiveData(): 总线 receive() → 对每个轴执行 cycleRun()（内含 statusSync）
 *
 * 设计原则：
 * - 依赖注入构造：通过构造函数注入 AxisConfig、Rtos 和具体传输（EtherCAT/MuJoCo）
 * - 禁止拷贝/赋值：Controller 持有 unique_ptr，不可拷贝
 * - 轴/IO/激光器通过 add*() 方法动态添加
 */
#pragma once

#include <memory>
#include <vector>

#include "Axis.h"
#include "Config.h"
#include "Io.h"
#include "Osal.h"
#include "shared_memory/ShmLayout.h"

// 传输层按编译期互斥模式选取具体类：REALTIME=EtherCAT 主站，SIMULATION=MuJoCo 总线，
// 其余模式无总线（send/receive 为空）。取消运行时多态，避免抽象层与 nullptr 死分支。
#ifdef REALTIME
#include "ethercat/EthercatMaster.h"
#elif defined(SIMULATION)
#include "mujoco/MujocoBus.h"
#endif

namespace ZrcsHardware {

class Controller {
public:
    // 禁止拷贝构造和赋值
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    /// 依赖注入构造函数
    /// @param config 轴参数配置（从 axis.xml 解析）
    /// @param rtos   实时线程封装（Xenomai/PreemptRt/Nativelinux）
    /// @param bus    硬件总线传输（EtherCAT/MuJoCo，由编译模式决定；其余模式无总线），可为空
    Controller(std::unique_ptr<AxisConfig> config,
               std::shared_ptr<Rtos> rtos
#ifdef REALTIME
               ,
               std::unique_ptr<EthercatMaster> bus = nullptr
#elif defined(SIMULATION)
               ,
               std::unique_ptr<MujocoBus> bus = nullptr
#endif
    )
        : rtos_(rtos)
        , axisConfig_(std::move(config))
#if defined(REALTIME) || defined(SIMULATION)
        , hardwareBus_(std::move(bus)) {}
#else
    {}
#endif

    ~Controller() = default;

    /// 添加一个轴（转移所有权）
    void addAxis(std::unique_ptr<Axis> axis) {
        axes_.push_back(std::move(axis));
    }

    /// 添加一个 IO 模块
    void addIo(std::unique_ptr<Io> io) {
        ios_.push_back(std::move(io));
    }


    /// 每周期后端：处理轴指令 → 发送总线数据
    void sendData();

    /// 每周期前端：接收总线数据 → 同步轴状态
    void receiveData();

#ifdef SIMULATION
    /// 供仿真数据节点（MujocoIdentPub 等）获取底层仿真对象；无 MuJoCo 时返回空。
    std::shared_ptr<MujocoSimulation> mujocoSimulation() const
    {
        return hardwareBus_ ? hardwareBus_->simulation() : nullptr;
    }
#endif

    void readIo() {}
    void writeIo() {}

    // ── 公开成员（RT 安全访问）────────────────────────────────

    std::vector<uint8_t>               outputData_;
    std::vector<uint8_t>               inputData_;
    std::shared_ptr<Rtos>              rtos_;
    std::vector<std::unique_ptr<Axis>> axes_;   // 所有轴的列表
    std::vector<std::unique_ptr<Io>>   ios_;    // 所有 IO 模块的列表

private:
    std::unique_ptr<AxisConfig>      axisConfig_;
#ifdef REALTIME
    std::unique_ptr<EthercatMaster>  hardwareBus_;
#elif defined(SIMULATION)
    std::unique_ptr<MujocoBus>       hardwareBus_;
#endif
};

} // namespace ZrcsHardware

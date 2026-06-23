/**
 * @file Controller.h
 * @brief 控制器聚合类 — 持有所有轴(Axis)、IO、激光器和硬件总线的实例
 *
 * 职责：
 * 1. 每 RT 周期调用 sendData()/receiveData() 完成数据收发
 * 2. sendData(): 对每个轴执行 cmdsProcessing() → updateMotionCmdsToServo() → 总线 send()
 * 3. receiveData(): 总线 receive() → 对每个轴执行 statusSync() → cycleRun()
 *
 * 设计原则：
 * - 依赖注入构造：通过构造函数注入 AxisConfig、Rtos 和 HardwareBus
 * - 禁止拷贝/赋值：Controller 持有 unique_ptr，不可拷贝
 * - 轴/IO/激光器通过 add*() 方法动态添加
 */
#pragma once

#include <memory>
#include <vector>

#include "AxisConfig.h"
#include "ControllerInterface.h"
#include "laser/LaserController.h"
#include "shared_memory/ShmLayout.h"

namespace ZrcsHardware {

class Controller {
public:
    // 禁止拷贝构造和赋值
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    /// 依赖注入构造函数
    /// @param config 轴参数配置（从 axis.xml 解析）
    /// @param rtos   实时线程封装（Xenomai/PreemptRt/Nativelinux）
    /// @param bus    硬件总线（EtherCAT 等），可为 nullptr
    Controller(std::unique_ptr<AxisConfig> config,
               std::shared_ptr<Rtos> rtos,
               std::unique_ptr<HardwareBus> bus = nullptr)
        : axisConfig_(std::move(config))
        , rtos_(rtos)
        , hardwareBus_(std::move(bus)) {}

    ~Controller() = default;

    /// 添加一个轴（转移所有权）
    void addAxis(std::unique_ptr<Axis> axis) {
        axes_.push_back(std::move(axis));
    }

    /// 添加一个 IO 模块
    void addIo(std::unique_ptr<Io> io) {
        ios_.push_back(std::move(io));
    }

    /// 设置激光器控制器
    void setLaser(std::unique_ptr<LaserController> laser) {
        laser_ = std::move(laser);
    }

    LaserController* laser() const { return laser_.get(); }

    /// 每周期后端：处理轴指令 → 发送总线数据
    void sendData();

    /// 每周期前端：接收总线数据 → 同步轴状态
    void receiveData();

    void bindSharedBlock(zrcs::SharedBlock* block) {
        if (hardwareBus_) {
            hardwareBus_->bindSharedBlock(block);
        }
    }

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
    std::unique_ptr<HardwareBus>     hardwareBus_;
    std::unique_ptr<LaserController> laser_;
};

} // namespace ZrcsHardware

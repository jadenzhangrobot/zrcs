/**
 * @file HardwareFactory.h
 * @brief 硬件工厂类 — 根据编译宏创建对应的控制器实例
 *
 * 通过条件编译（REALTIME / SIMULATION / STANDARD）自动选择:
 * - REALTIME:  EtherCAT 主站 + Xenomai RTOS + EthercatMotor
 * - SIMULATION: VirtualServo 虚拟伺服
 * - STANDARD: VirtualServo 虚拟伺服
 *
 * 非 REALTIME 时使用 Linux 原生线程作为 RTOS。
 *
 * @author zhangyongjing
 */
#pragma once

#include <memory>
#include <string>

#include "controller/Controller.h"

namespace ZrcsHardware {

/**
 * @brief 硬件工厂
 *
 * 静态工厂方法 createController() 根据编译时宏定义创建完整的
 * Controller 实例，包括:
 * - 从 axis.xml 解析轴配置 (AxisConfig)
 * - 创建对应的 RTOS 实例 (xenomai 或 Nativelinux)
 * - 创建硬件总线实例 (EthercatMaster 或 nullptr)
 * - 为每个轴创建对应的伺服驱动器实例
 * - 为非 MOTOR 类型的 EtherCAT 从站创建 IO 对象
 */
class HardwareFactory {
public:
    /**
     * @brief 创建 Controller 实例
     * @param projectName 项目名称前缀，用于定位 XML 配置文件
     * @return 完全配置好的 Controller 实例
     */
    static std::unique_ptr<Controller> createController(const std::string& projectName = "");
};

} // namespace ZrcsHardware

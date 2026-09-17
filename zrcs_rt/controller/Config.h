#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Axis.h"

namespace ZrcsHardware {

// AxisPara / ServoPara 及各类枚举已随域 headers 分发：
//   - AxisPara、MC_ERROR_CODE、MC_DIRECTION、MC_BUFFER_MODE、MC_MOTION_MODE -> Axis.h
//   - ServoPara、MC_SERVO_CODE、MC_SERVO_CONTROL_MODE、Cia402Mode           -> Servo.h
//   - MC_NETWORK_ERROR_CODE                                                  -> Osal.h
// 本头文件保留配置文件的兼容包装类（从 XML 加载为运行时参数结构）。

/**
 * @brief 将 axis.xml 加载为运行时 AxisPara 的兼容包装类。
 *
 * 新的启动流程通常直接使用 ConfigManager。保留这个包装类，是为了让旧调用点
 * 仍然可以通过文件名构造 AxisConfig，同时内部走新的强类型解析逻辑。
 */
class AxisConfig {
public:
    std::vector<AxisPara> axisParas;

    AxisConfig() = default;
    explicit AxisConfig(const std::string& xmlFileName);
    explicit AxisConfig(std::vector<AxisPara> axisParasIn)
        : axisParas(std::move(axisParasIn)) {}
};

/**
 * @brief 将 servo.xml 加载为运行时 ServoPara 的兼容包装类。
 */
class ServoConfig {
public:
    std::vector<ServoPara> servoParas;

    ServoConfig() = default;
    explicit ServoConfig(const std::string& xmlFileName);
    explicit ServoConfig(std::vector<ServoPara> servoParasIn)
        : servoParas(std::move(servoParasIn)) {}
};

} // namespace ZrcsHardware

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Global.h"

namespace ZrcsHardware {

/**
 * @brief Controller/Axis 运行时使用的轴参数。
 *
 * 该结构刻意和 zrcs::config::AxisConfigData 分开：config 命名空间描述文件数据，
 * 这里描述控制器运行时需要的形态，并可以保留 frequency 这类只属于控制器的字段。
 */
struct AxisPara {
    uint32_t axisId = 0;
    std::string axisName;
    std::vector<uint32_t> servoSlaveIds;
    double maxVel = 0.0;
    double maxAcc = 0.0;
    double maxJerk = 0.0;
    double posPositiveLimit = 0.0;
    double posNegativeLimit = 0.0;
    double maxPosDiff = 0.0;
    double frequency = 0.0;
};

/**
 * @brief 绑定到 Axis 的运行时伺服参数。
 *
 * 一个 Axis 可以拥有多个 ServoPara。第 i 个 ServoPara 对应第 i 个 push 到 Axis
 * 的 Servo 对象，因此命令和反馈换算可以分别使用每个驱动器自己的模式、编码器比例
 * 和安装方向。
 */
struct ServoPara {
    uint32_t slaveId = 0;
    MC_SERVO_CONTROL_MODE mode = MC_SERVO_CONTROL_MODE::mcServoControlModePosition;
    uint64_t encoderCountPerUnit = 1;
    int direction = 1;
    double homePos = 0.0;
    double posOffset = 0.0;
    double velFactor = 1.0;
};

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

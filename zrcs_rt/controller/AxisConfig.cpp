#include "controller/AxisConfig.h"

#include "config/ConfigSerializer.h"

#include <stdexcept>

namespace ZrcsHardware {
namespace {

// 配置文件里用可读字符串保存伺服模式，运行时代码沿用已有的
// MC_SERVO_CONTROL_MODE 枚举。把转换集中在这里，可以统一校验支持的模式名称。
MC_SERVO_CONTROL_MODE parseMode(const std::string& mode)
{
    if (mode == "position") {
        return MC_SERVO_CONTROL_MODE::mcServoControlModePosition;
    }
    if (mode == "velocity") {
        return MC_SERVO_CONTROL_MODE::mcServoControlModeVelocity;
    }
    if (mode == "torque") {
        return MC_SERVO_CONTROL_MODE::mcServoControlModeTorque;
    }
    throw std::runtime_error("Unsupported servo control mode: " + mode);
}

AxisPara toAxisPara(const zrcs::config::AxisConfigData& data)
{
    // 这里只复制轴级字段。伺服比例和模式通过 ServoPara 加载，
    // 这样多驱轴可以使用每个驱动器自己的参数。
    AxisPara para;
    para.axisId = data.axisId;
    para.axisName = data.axisName;
    para.servoSlaveIds = data.servoSlaveIds;
    para.maxVel = data.maxVel;
    para.maxAcc = data.maxAcc;
    para.maxJerk = data.maxJerk;
    para.posPositiveLimit = data.posPositiveLimit;
    para.posNegativeLimit = data.posNegativeLimit;
    para.maxPosDiff = data.maxPosDiff;
    return para;
}

ServoPara toServoPara(const zrcs::config::ServoConfigData& data)
{
    // 将文件配置转换为 Axis 使用的控制器运行时结构。
    ServoPara para;
    para.slaveId = data.slaveId;
    para.mode = parseMode(data.mode);
    para.encoderCountPerUnit = data.encoderCountPerUnit;
    para.direction = data.direction;
    para.homePos = data.homePos;
    para.posOffset = data.posOffset;
    para.velFactor = data.velFactor;
    return para;
}

} // namespace

AxisConfig::AxisConfig(const std::string& xmlFileName)
{
    // 旧代码路径的兼容入口。现在它和 ConfigManager 使用同一套序列化工具，
    // 不再依赖已经删除的 XmlParsing 包装。
    auto file = zrcs::config::loadXml<zrcs::config::AxisConfigFile>(xmlFileName);
    axisParas.reserve(file.axes.size());
    for (const auto& axis : file.axes) {
        axisParas.push_back(toAxisPara(axis));
    }
}

ServoConfig::ServoConfig(const std::string& xmlFileName)
{
    // 旧代码路径的兼容入口。新代码应优先使用 ConfigManager，
    // 以便在创建硬件对象前完成跨文件校验。
    auto file = zrcs::config::loadXml<zrcs::config::ServoConfigFile>(xmlFileName);
    servoParas.reserve(file.servos.size());
    for (const auto& servo : file.servos) {
        servoParas.push_back(toServoPara(servo));
    }
}

} // namespace ZrcsHardware

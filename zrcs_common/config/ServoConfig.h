#pragma once

#include "ConfigSerializer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace zrcs::config {

/**
 * @brief 从 servo.xml 加载的伺服级配置。
 *
 * slaveId 在 REALTIME 模式下用于关联 ethercat.xml 中的从站，在
 * STANDARD/SIMULATION 模式下用于创建对应的虚拟伺服。encoderCountPerUnit
 * 保留在伺服级别，因为同一台设备上的不同驱动器可能有不同的编码器换算比例。
 * direction 描述伺服编码器正方向相对逻辑轴正方向的符号，反装双驱伺服写 -1。
 */
struct ServoConfigData {
    uint32_t slaveId = 0;
    std::string mode = "position";
    uint64_t encoderCountPerUnit = 1;
    int direction = 1;
    double homePos = 0.0;
    double posOffset = 0.0;
    double velFactor = 1.0;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(slaveId), CEREAL_NVP(mode), CEREAL_NVP(encoderCountPerUnit),
           CEREAL_NVP(direction), CEREAL_NVP(homePos), CEREAL_NVP(posOffset),
           CEREAL_NVP(velFactor));
    }
};

/// servo.xml 的顶层数据对象，每个元素描述一个伺服驱动器。
struct ServoConfigFile {
    std::vector<ServoConfigData> servos;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(servos));
    }
};

/// servo.xml 根节点固定为 "servoConfig"。
template <>
struct XmlRoot<ServoConfigFile> {
    static constexpr const char* name() { return "servoConfig"; }
};

} // namespace zrcs::config

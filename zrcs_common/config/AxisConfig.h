#pragma once

#include "ConfigSerializer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace zrcs::config {

/**
 * @brief 从 axis.xml 加载的轴级配置。
 *
 * 这里描述的是“逻辑运动轴”的参数。伺服自己的模式、编码器换算等参数
 * 放在 ServoConfigData 中，轴这里只保存它绑定的 slaveId 列表。这样一个
 * 逻辑轴可以引用多个伺服，用于双驱轴、龙门轴等机械结构。
 */
struct AxisConfigData {
    uint32_t axisId = 0;
    std::string axisName;
    std::vector<uint32_t> servoSlaveIds;
    double maxVel = 0.0;
    double maxAcc = 0.0;
    double maxJerk = 0.0;
    double posPositiveLimit = 0.0;
    double posNegativeLimit = 0.0;
    double maxPosDiff = 0.0;
    double lead = 0.0;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(axisId), CEREAL_NVP(axisName), CEREAL_NVP(servoSlaveIds),
           CEREAL_NVP(maxVel), CEREAL_NVP(maxAcc), CEREAL_NVP(maxJerk),
           CEREAL_NVP(posPositiveLimit), CEREAL_NVP(posNegativeLimit),
           CEREAL_NVP(maxPosDiff), CEREAL_NVP(lead));
    }
};

/// axis.xml 的顶层数据对象，包一层结构是为了给 cereal 一个稳定的根对象。
struct AxisConfigFile {
    std::vector<AxisConfigData> axes;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(axes));
    }
};

/// axis.xml 根节点固定为 "axisConfig"。
template <>
struct XmlRoot<AxisConfigFile> {
    static constexpr const char* name() { return "axisConfig"; }
};

} // namespace zrcs::config

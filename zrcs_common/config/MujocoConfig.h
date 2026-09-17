#pragma once

#include "ConfigSerializer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace zrcs::config {

/**
 * @brief mujoco.xml 中单个仿真伺服的可序列化数据。
 *
 * 旧属性风格 XML 允许省略 kp/kd/qposScale/qposOffset，依赖默认值。迁移到
 * cereal 后文件会显式写出这些默认值（cereal XML 要求字段齐全），语义不变。
 */
struct MujocoServoConfigData {
    uint32_t slaveId = 0;
    std::string joint;
    double kp = 200.0;
    double kd = 20.0;
    double qposScale = 1.0;
    double qposOffset = 0.0;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(slaveId), CEREAL_NVP(joint), CEREAL_NVP(kp), CEREAL_NVP(kd),
           CEREAL_NVP(qposScale), CEREAL_NVP(qposOffset));
    }
};

/**
 * @brief mujoco.xml 的可序列化数据层。
 *
 * model 是相对项目目录的 MJCF 路径（运行时解析成绝对路径并检查存在）；
 * timestepMs/substeps/kinematicOnly 的校验由 RT 层的 ZrcsHardware::MujocoConfig
 * 负责，序列化本身不强制数值语义。
 */
struct MujocoConfigFile {
    std::string model;
    double timestepMs = 1.0;
    int substeps = 1;
    bool kinematicOnly = false;
    std::vector<MujocoServoConfigData> servos;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(model), CEREAL_NVP(timestepMs), CEREAL_NVP(substeps),
           CEREAL_NVP(kinematicOnly), CEREAL_NVP(servos));
    }
};

/// mujoco.xml 根节点固定为 "mujocoConfig"。
template <>
struct XmlRoot<MujocoConfigFile> {
    static constexpr const char* name() { return "mujocoConfig"; }
};

} // namespace zrcs::config

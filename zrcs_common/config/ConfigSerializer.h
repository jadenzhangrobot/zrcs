#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

namespace zrcs::config {

/**
 * @brief model.xml 中保存的位姿结构。
 *
 * 运行时运动学使用 Eigen::Matrix4d，但配置文件里只保存
 * x/y/z/rx/ry/rz 六个标量。这样 XML 更容易人工编辑，也不会让配置格式
 * 依赖 Eigen 矩阵的内存布局或 cereal 对 Eigen 的序列化支持。
 */
struct PoseConfig {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z),
           CEREAL_NVP(rx), CEREAL_NVP(ry), CEREAL_NVP(rz));
    }
};

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

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(axisId), CEREAL_NVP(axisName), CEREAL_NVP(servoSlaveIds),
           CEREAL_NVP(maxVel), CEREAL_NVP(maxAcc), CEREAL_NVP(maxJerk),
           CEREAL_NVP(posPositiveLimit), CEREAL_NVP(posNegativeLimit),
           CEREAL_NVP(maxPosDiff));
    }
};

/**
 * @brief 从 servo.xml 加载的伺服级配置。
 *
 * slaveId 在 REALTIME 模式下用于关联 ethercat.xml 中的从站，在
 * STANDARD/SIMULATION 模式下用于创建对应的虚拟伺服。encoderCountPerUnit
 * 保留在伺服级别，因为同一台设备上的不同驱动器可能有不同的编码器换算比例。
 */
struct ServoConfigData {
    uint32_t slaveId = 0;
    std::string mode = "position";
    uint64_t encoderCountPerUnit = 1;
    double homePos = 0.0;
    double posOffset = 0.0;
    double velFactor = 1.0;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(slaveId), CEREAL_NVP(mode), CEREAL_NVP(encoderCountPerUnit),
           CEREAL_NVP(homePos), CEREAL_NVP(posOffset), CEREAL_NVP(velFactor));
    }
};

/**
 * @brief 从 model.xml 加载的单个模型关节配置。
 *
 * axisId 用来把运动学关节关联回 axis.xml 中的逻辑轴。ConfigManager 会在
 * RT 启动前统一校验这个关系，避免模型运动命令执行到一半才发现轴不存在。
 */
struct ModelJointConfigData {
    int axisId = 0;
    std::string type = "revolute";
    double offset = 0.0;
    double dh_a = 0.0;
    double dh_alpha = 0.0;
    double dh_d = 0.0;
    double dh_theta = 0.0;
    char axis = 'X';

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(axisId), CEREAL_NVP(type), CEREAL_NVP(offset),
           CEREAL_NVP(dh_a), CEREAL_NVP(dh_alpha), CEREAL_NVP(dh_d),
           CEREAL_NVP(dh_theta), CEREAL_NVP(axis));
    }
};

/**
 * @brief 从 model.xml 加载的机器人模型描述。
 *
 * 序列化结构刻意只包含普通标量和 vector。ModelConfig 会在加载后把
 * PoseConfig 转成 Eigen 矩阵，把 ModelJointConfigData 转成运行时的
 * ModelJoint，从而让配置序列化和运行时数学类型解耦。
 */
struct ModelConfigData {
    std::string name;
    std::string type;
    int dof = 0;
    PoseConfig baseFrame;
    PoseConfig toolFrame;
    double basePlatformRadius = 0.0;
    double mobilePlatformRadius = 0.0;
    double upperArmLength = 0.0;
    double lowerArmLength = 0.0;
    std::vector<ModelJointConfigData> joints;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(name), CEREAL_NVP(type), CEREAL_NVP(dof),
           CEREAL_NVP(baseFrame), CEREAL_NVP(toolFrame),
           CEREAL_NVP(basePlatformRadius), CEREAL_NVP(mobilePlatformRadius),
           CEREAL_NVP(upperArmLength), CEREAL_NVP(lowerArmLength),
           CEREAL_NVP(joints));
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

/// servo.xml 的顶层数据对象，每个元素描述一个伺服驱动器。
struct ServoConfigFile {
    std::vector<ServoConfigData> servos;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(servos));
    }
};

/// model.xml 的顶层数据对象，一个项目中可以放多个具名模型。
struct ModelConfigFile {
    std::vector<ModelConfigData> models;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(models));
    }
};

/**
 * @brief 加载一个强类型 XML 配置对象。
 *
 * 这个工具只用于 axis/servo/model 这类业务配置。ethercat.xml 属于协议和
 * 拓扑描述，由 SlaveConfig 单独使用 tinyxml2 解析。
 */
template <typename T>
T loadXml(const std::filesystem::path& path);

/**
 * @brief 保存一个强类型 XML 配置对象。
 *
 * 下面的特化函数会固定文件级根节点名称
 * (axisConfig/servoConfig/modelConfig)，避免调用点影响文件格式。
 */
template <typename T>
void saveXml(const std::filesystem::path& path, const T& value);

template <>
AxisConfigFile loadXml<AxisConfigFile>(const std::filesystem::path& path);

template <>
ServoConfigFile loadXml<ServoConfigFile>(const std::filesystem::path& path);

template <>
ModelConfigFile loadXml<ModelConfigFile>(const std::filesystem::path& path);

template <>
void saveXml<AxisConfigFile>(const std::filesystem::path& path, const AxisConfigFile& value);

template <>
void saveXml<ServoConfigFile>(const std::filesystem::path& path, const ServoConfigFile& value);

template <>
void saveXml<ModelConfigFile>(const std::filesystem::path& path, const ModelConfigFile& value);

} // namespace zrcs::config

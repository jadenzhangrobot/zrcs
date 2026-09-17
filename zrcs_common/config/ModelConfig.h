#pragma once

#include "ConfigSerializer.h"

#include <cstdint>
#include <string>
#include <vector>

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

/// model.xml 的顶层数据对象，一个项目中可以放多个具名模型。
struct ModelConfigFile {
    std::vector<ModelConfigData> models;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(models));
    }
};

/// model.xml 根节点固定为 "modelConfig"。
template <>
struct XmlRoot<ModelConfigFile> {
    static constexpr const char* name() { return "modelConfig"; }
};

} // namespace zrcs::config

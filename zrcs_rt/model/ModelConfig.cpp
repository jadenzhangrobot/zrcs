#include "model/ModelConfig.h"

#include "config/ModelConfig.h"

#include <stdexcept>

namespace {

// model.xml 中用可读文本保存关节类型，运行时模型类使用已有枚举。
// 因此解析和校验都集中在配置边界完成。
JointType parseJointType(const std::string& type)
{
    if (type == "revolute") {
        return JointType::REVOLUTE;
    }
    if (type == "prismatic") {
        return JointType::PRISMATIC;
    }
    throw std::runtime_error("Unsupported joint type: " + type);
}

ModelJoint toModelJoint(const zrcs::config::ModelJointConfigData& data)
{
    // 保持旧的 ModelJoint 运行时结构，使 ModelFactory 和具体 RobotModel
    // 实现不需要感知 cereal 配置类型。
    ModelJoint joint;
    joint.axisId = data.axisId;
    joint.type = parseJointType(data.type);
    joint.offset = data.offset;
    joint.dh_a = data.dh_a;
    joint.dh_alpha = data.dh_alpha;
    joint.dh_d = data.dh_d;
    joint.dh_theta = data.dh_theta;
    joint.axis = data.axis;
    return joint;
}

Eigen::Matrix4d toPose(const zrcs::config::PoseConfig& pose)
{
    // 文件中保存 x/y/z/rx/ry/rz，加载后再转换为齐次变换矩阵，
    // 让 model.xml 不依赖 Eigen 的序列化方式。
    return RobotModel::poseFromXYZRPY(
        pose.x, pose.y, pose.z, pose.rx, pose.ry, pose.rz);
}

ModelParam toModelParam(const zrcs::config::ModelConfigData& data)
{
    // ModelParam 是 ModelFactory 消费的旧运行时结构。通过这个适配层，
    // 可以引入新的 cereal 文件格式，而不需要改动后续运动学栈。
    ModelParam param;
    param.name = data.name;
    param.type = data.type;
    param.dof = data.dof;
    param.baseTf = toPose(data.baseFrame);
    param.toolTf = toPose(data.toolFrame);
    param.basePlatformRadius = data.basePlatformRadius;
    param.mobilePlatformRadius = data.mobilePlatformRadius;
    param.upperArmLength = data.upperArmLength;
    param.lowerArmLength = data.lowerArmLength;
    param.joints.reserve(data.joints.size());
    for (const auto& joint : data.joints) {
        param.joints.push_back(toModelJoint(joint));
    }
    return param;
}

} // namespace

ModelConfig::ModelConfig(const std::string& xmlFileName)
{
    try {
        // model.xml 使用和 axis.xml、servo.xml 相同的通用序列化入口。
        // 正常 RT 启动时，ConfigManager 会在构造本类前完成跨文件校验。
        auto file = zrcs::config::loadXml<zrcs::config::ModelConfigFile>(xmlFileName);
        modelParams.reserve(file.models.size());
        for (const auto& model : file.models) {
            modelParams.push_back(toModelParam(model));
        }
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Failed to parse model config: ") + e.what());
    }
}

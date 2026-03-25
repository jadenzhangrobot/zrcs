/**
 * @file modelConfig.h
 * @brief 从 model.xml 解析机器人模型配置
 *
 * 参照 axisConfig.h 的模式，使用 XmlParsing + TreeNode 解析 XML。
 * 支持多模型配置，每个模型独立配置类型、关节、坐标系等。
 */
#ifndef MODEL_CONFIG_H
#define MODEL_CONFIG_H

#include "xml/xmlParsing.h"
#include "robotModel.h"
#include <string>
#include <vector>
#include <stdexcept>

/**
 * @brief 模型参数结构体
 * 从 XML 解析出的完整模型描述
 */
struct ModelParam
{
    std::string name;
    std::string type;     // "serial", "delta", "cartesian"
    int dof = 0;
    std::vector<ModelJoint> joints;

    // 坐标系
    Eigen::Matrix4d baseTf = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d toolTf = Eigen::Matrix4d::Identity();

    // Delta并联专用参数
    double basePlatformRadius   = 0;
    double mobilePlatformRadius = 0;
    double upperArmLength       = 0;
    double lowerArmLength       = 0;
};

/**
 * @brief 模型配置解析器
 *
 * XML格式示例:
 * <modelConfig>
 *   <model-1>
 *     <attribute name="ur5" type="serial" dof="6"/>
 *     <baseFrame x="0" y="0" z="0" rx="0" ry="0" rz="0"/>
 *     <toolFrame x="0" y="0" z="0.15" rx="0" ry="0" rz="0"/>
 *     <joint-0 axisId="0" type="revolute" offset="0" dh_a="0" .../>
 *   </model-1>
 * </modelConfig>
 */
class ModelConfig : private XmlParsing
{
public:
    std::vector<ModelParam> modelParams;

    ModelConfig(const std::string& xmlFileName) : XmlParsing(xmlFileName)
    {
        try
        {
            for (auto& modelNode : tree->root->children)
            {
                ModelParam param;
                auto* node = modelNode.second;

                // 解析 <attribute> 节点
                if (node->children.find("attribute") != node->children.end())
                {
                    auto* attr = node->children["attribute"];
                    param.name = attr->attribute["name"];
                    param.type = attr->attribute["type"];
                    param.dof  = std::stoi(attr->attribute["dof"]);
                }

                // 解析 <baseFrame> 节点 (可选)
                if (node->children.find("baseFrame") != node->children.end())
                {
                    auto* bf = node->children["baseFrame"];
                    param.baseTf = RobotModel::poseFromXYZRPY(
                        std::stod(bf->attribute["x"]),
                        std::stod(bf->attribute["y"]),
                        std::stod(bf->attribute["z"]),
                        std::stod(bf->attribute["rx"]),
                        std::stod(bf->attribute["ry"]),
                        std::stod(bf->attribute["rz"]));
                }

                // 解析 <toolFrame> 节点 (可选)
                if (node->children.find("toolFrame") != node->children.end())
                {
                    auto* tf = node->children["toolFrame"];
                    param.toolTf = RobotModel::poseFromXYZRPY(
                        std::stod(tf->attribute["x"]),
                        std::stod(tf->attribute["y"]),
                        std::stod(tf->attribute["z"]),
                        std::stod(tf->attribute["rx"]),
                        std::stod(tf->attribute["ry"]),
                        std::stod(tf->attribute["rz"]));
                }

                // 解析 <geometry> 节点 (Delta并联专用)
                if (node->children.find("geometry") != node->children.end())
                {
                    auto* geo = node->children["geometry"];
                    param.basePlatformRadius   = std::stod(geo->attribute["basePlatformRadius"]);
                    param.mobilePlatformRadius = std::stod(geo->attribute["mobilePlatformRadius"]);
                    param.upperArmLength       = std::stod(geo->attribute["upperArmLength"]);
                    param.lowerArmLength       = std::stod(geo->attribute["lowerArmLength"]);
                }

                // 解析 joint-N 节点
                for (int i = 0; i < param.dof; i++)
                {
                    std::string jointKey = "joint-" + std::to_string(i);
                    if (node->children.find(jointKey) == node->children.end())
                    {
                        break;
                    }

                    auto* jn = node->children[jointKey];
                    ModelJoint joint;

                    joint.axisId = std::stoi(jn->attribute["axisId"]);

                    std::string typeStr = jn->attribute["type"];
                    if (typeStr == "revolute")
                    {
                        joint.type = JointType::REVOLUTE;
                    }
                    else if (typeStr == "prismatic")
                    {
                        joint.type = JointType::PRISMATIC;
                    }

                    if (jn->attribute.count("offset"))
                    {
                        joint.offset = std::stod(jn->attribute["offset"]);
                    }

                    // DH参数 (串联专用)
                    if (jn->attribute.count("dh_a"))
                    {
                        joint.dh_a = std::stod(jn->attribute["dh_a"]);
                    }
                    if (jn->attribute.count("dh_alpha"))
                    {
                        joint.dh_alpha = std::stod(jn->attribute["dh_alpha"]);
                    }
                    if (jn->attribute.count("dh_d"))
                    {
                        joint.dh_d = std::stod(jn->attribute["dh_d"]);
                    }
                    if (jn->attribute.count("dh_theta"))
                    {
                        joint.dh_theta = std::stod(jn->attribute["dh_theta"]);
                    }

                    // 笛卡尔轴映射
                    if (jn->attribute.count("axis"))
                    {
                        joint.axis = jn->attribute["axis"][0];
                    }

                    param.joints.push_back(joint);
                }

                modelParams.push_back(param);
            }
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(std::string("Failed to parse model config: ") + e.what());
        }
    }
};

#endif // MODEL_CONFIG_H

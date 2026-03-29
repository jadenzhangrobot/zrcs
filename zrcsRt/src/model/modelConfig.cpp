/**
 * @file modelConfig.cpp
 * @brief 模型配置解析器实现
 */
#include "model/modelConfig.h"

ModelConfig::ModelConfig(const std::string& xmlFileName) : XmlParsing(xmlFileName)
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

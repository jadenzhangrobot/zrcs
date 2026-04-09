/**
 * @file modelConfig.h
 * @brief 从 model.xml 解析机器人模型配置
 *
 * 参照 AxisConfig.h 的模式，使用 XmlParsing + TreeNode 解析 XML。
 * 支持多模型配置，每个模型独立配置类型、关节、坐标系等。
 */
#pragma once

#include "xml/XmlParsing.h"
#include "RobotModel.h"
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

    ModelConfig(const std::string& xmlFileName);
};


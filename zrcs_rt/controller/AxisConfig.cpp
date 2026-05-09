/**
 * @file AxisConfig.cpp
 * @brief AxisConfig 类方法实现
 *
 * 从 axis.xml 解析每个轴的属性（ID、名称、控制模式、编码器分辨率、
 * 运动限制参数等），并填充到 AxisPara 向量中。
 *
 * XML 节点结构（示例）:
 * @code{.xml}
 * <axis attribute="id:0 name:X slaveId:0 mode:position">
 *   <machine value="X轴"/>
 *   <motion maxVel="100" maxAcc="500" maxJerk="5000" maxPos="300" minPos="-300" maxPosDiff="1"/>
 *   <encoder posFactor="10000"/>
 * </axis>
 * @endcode
 */
#include "controller/AxisConfig.h"

#include <stdexcept>

namespace ZrcsHardware {

AxisConfig::AxisConfig(const std::string& xmlFileName)
    : XmlParsing(xmlFileName)
{
    try {
        for (auto child : tree->root->children) {
            AxisPara axisPara;
            axisPara.axisId = std::stoi(child.second->children["attribute"]->attribute["id"]);
            axisPara.axisName = child.second->children["attribute"]->attribute["name"];
            axisPara.slaveId = std::stoi(child.second->children["attribute"]->attribute["slaveId"]);

            std::string mode = child.second->children["attribute"]->attribute["mode"];
            if (mode == "position") {
                axisPara.mode = MC_SERVO_CONTROL_MODE::mcServoControlModePosition;
            } else if (mode == "velocity") {
                axisPara.mode = MC_SERVO_CONTROL_MODE::mcServoControlModeVelocity;
            }

            axisPara.axisName = child.second->children["machine"]->attribute["value"];
            axisPara.maxVel = std::stod(child.second->children["motion"]->attribute["maxVel"]);
            axisPara.maxAcc = std::stod(child.second->children["motion"]->attribute["maxAcc"]);
            axisPara.maxJerk = std::stod(child.second->children["motion"]->attribute["maxJerk"]);
            axisPara.posPositiveLimit = std::stod(child.second->children["motion"]->attribute["maxPos"]);
            axisPara.posNegativeLimit = std::stod(child.second->children["motion"]->attribute["minPos"]);
            axisPara.maxPosDiff = std::stod(child.second->children["motion"]->attribute["maxPosDiff"]);
            axisPara.encoderCountPerUnit = std::stoll(child.second->children["encoder"]->attribute["posFactor"]);
            axisParas.push_back(axisPara);
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to read axis parameters");
    }
}

} // namespace ZrcsHardware

/*
 * @Description: AxisConfig class method implementations
 */
#include "controller/AxisConfig.h"
#include <stdexcept>

namespace ZrcsHardware {

AxisConfig::AxisConfig(const std::string& xmlFileName):XmlParsing(xmlFileName)
{
    try {
        for (auto child:tree->root->children)
        {
            AxisPara axisPara;
            axisPara.axisId =std::stoi(child.second->children["attribute"]->attribute["id"]);
            axisPara.axisName =child.second->children["attribute"]->attribute["name"];
            axisPara.slaveId =std::stoi(child.second->children["attribute"]->attribute["slaveId"]);
            std::string mode = child.second->children["attribute"]->attribute["mode"];
            if(mode == "position")
            {
                axisPara.mode = MC_SERVO_CONTROL_MODE::mcServoControlModePosition;
            }
            else if(mode== "velocity")
            {
                axisPara.mode = MC_SERVO_CONTROL_MODE::mcServoControlModeVelocity;
            }
            axisPara.axisName = child.second->children["machine"]->attribute["value"];
            axisPara.maxVel = std::stod(child.second->children["motion"]->attribute["maxVel"]);
            axisPara.maxAcc = std::stod( child.second->children["motion"]->attribute["maxAcc"]);
            axisPara.maxJerk = std::stod( child.second->children["motion"]->attribute["maxJerk"]);
            axisPara.posPositiveLimit = std::stod( child.second->children["motion"]->attribute["maxPos"]);
            axisPara.posNegativeLimit = std::stod( child.second->children["motion"]->attribute["minPos"]);
            axisPara.maxPosDiff = std::stod( child.second->children["motion"]->attribute["maxPosDiff"]);
            axisPara.encoderCountPerUnit= std::stoll(child.second->children["encoder"]->attribute["posFactor"]);
            axisParas.push_back(axisPara);
        }
    }
    catch (const std::exception& e) {
        throw std::runtime_error("Failed to read axis parameters");
    }
}

} // namespace ZrcsHardware

#ifndef PARAMETERREAD_H
#define PARAMETERREAD_H
#include "xml/xmlParsing.h"
#include "config/parameter.h"
#include "global.h"
#include <boost/container/options.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
using namespace tinyxml2;
namespace ZrcsHardware 
{ 
        typedef struct {
        uint32_t axisId;
        uint32_t slaveId;
        std::string axisName;
        MC_SERVO_CONTROL_MODE mode;
        uint64_t encoderCountPerUnit;
        double maxVel;
        double maxAcc;
        double maxJerk;
        double posPositiveLimit;
        double posNegativeLimit;
        double maxPosDiff;
        double frequency;
    }AxisPara;
    class AxisConfig:private XmlParsing
    {      
            public:
            std::vector<AxisPara> axisParas;
            AxisConfig(const std::string& xmlFileName):XmlParsing(xmlFileName)
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
    };
}
#endif
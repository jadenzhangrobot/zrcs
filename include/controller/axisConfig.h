#ifndef PARAMETERREAD_H
#define PARAMETERREAD_H
#include "common/xmlParsing.h"
#include "global.h"
#include <cstdint>
#include <string>
#include <vector>
using namespace tinyxml2;
namespace ZrcsHardware 
{ 
        typedef struct {
        uint32_t axisId;
        uint32_t slaveId;
        std::string axisName;
        MC_SERVO_CONTROL_MODE mode_ ;
        uint64_t encoder_count_per_unit_;
        uint64_t node_buffer_size_;
        bool max_vel_ ;
        double min_vel_;
        bool max_acc_ ;
        double min_acc_;
        double pos_positive_limit_;
        double pos_negative_limit_ ;
        double frequency_ ;
    }AxisPara;
    class AxisConfig:private XmlParsing
    {      
            public:
            std::vector<AxisPara> axisParas;
            AxisConfig(const std::string& xmlFileName):XmlParsing(xmlFileName)
            {
                for (auto child:tree->root->children)
                {
                     AxisPara axisPara;
                     axisPara.axisId =std::stoi(child.second->attribute["ID"]);
                     axisPara.axisName = child.second->children["machine"]->attribute["value"];
                     axisPara.slaveId = std::stoi(child.second->attribute["slaveId"]);
                      std::string mode = child.second->attribute["mode"];
                     if(mode == "position")
                     {
                        axisPara.mode_ = MC_SERVO_CONTROL_MODE::mcServoControlModePosition;
                     }
                     else if(mode== "velocity")
                     {
                        axisPara.mode_ = MC_SERVO_CONTROL_MODE::mcServoControlModeVelocity;
                     }
                     axisPara.max_vel_ = std::stod(child.second->children["motion"]->attribute["max_vel"]);
                     axisPara.max_acc_ = std::stod( child.second->children["motion"]->attribute["max_acc"]);
                     axisPara.pos_positive_limit_ = std::stod( child.second->children["motion"]->attribute["max_pos"]);
                     axisPara.pos_negative_limit_ = std::stod( child.second->children["motion"]->attribute["min_pos"]);

                     axisPara.encoder_count_per_unit_= std::stoll(child.second->children["encoder"]->attribute["pos_factor"]);
                     axisParas.push_back(axisPara);                                                        
                }
                          
            }       
    };
}
#endif
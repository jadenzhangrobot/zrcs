#ifndef PARAMETERREAD_H
#define PARAMETERREAD_H
#include <vector>
#include <cstdint>
#include <filesystem>
#include "tinyxml2.h"
using namespace tinyxml2;
namespace ZrcsSystem 
{ 
    typedef struct {
    int id; //电机ID
    std::string Type; //电机类型
    int slave; //从站ID
    double  Pitch;   //螺距：每转动一圈的移动距离 
    bool  Direction;  // 旋转方向：正向或负向 -->
    double Ratio; //传动比    
    double Backlash;//反向间隙
    double max_pos = 1.0; //正向软限位
    double min_pos = -1.0;//负向软限位
    double max_vel = 1.0;//最大速度
    double min_vel = -1.0; //最小速度
    double max_acc = 1.0;//最大加速度
    double min_acc = -1.0;//最小加速度             
    double max_pos_following_error = 1.0;//最大位置跟随误差
    double max_vel_following_error = 1.0;//最大速度跟随误差
    double pos_factor = 1.0;//编码器分辨率    
    double pos_offset = 0.0;//位置偏移
    double home_pos = 0.0;//原点位置
    double vel_factor = 1.0;//速度比例
    double target_vel_ = 0;//目标速度
    double target_toq_ = 0;//目标力矩
    double offset_vel_ = 0;//速度偏移
    double offset_toq_ = 0;//力矩偏移
    } AxisPara;
    class AxisConfig
    { 
            tinyxml2::XMLDocument axisDoc;
            public:
            std::vector<AxisPara> axisParas;         
          

            AxisConfig(const std::string& projectPath)
            {
                std::string axisConfigPath = projectPath + "config/axis.xml";
                if (axisDoc.LoadFile(axisConfigPath.c_str()) == tinyxml2::XML_SUCCESS) 
                {
                    XMLElement *root = axisDoc.FirstChildElement("AxisConfig");
                    if (root)
                    {       
                        for (XMLElement *axis = root->FirstChildElement("Axis"); axis; axis = axis->NextSiblingElement("Axis"))
                        {
                            AxisPara axisPara;
                            if (const XMLAttribute *attr = axis->FindAttribute("ID"))
                                axisPara.id = std::stoi(attr->Value());
                            if (const XMLAttribute *attr = axis->FindAttribute("Type"))
                                 axisPara.Type = attr->Value();
                            if (const XMLAttribute *attr = axis->FindAttribute("slave"))
                                axisPara.slave = std::stoi(attr->Value());
                            
                            if (XMLElement *pitchElement = axis->FirstChildElement("Pitch"))
                                if (const char *text = pitchElement->GetText())
                                    axisPara.Pitch = std::stod(text);
                            if (XMLElement *directionElement = axis->FirstChildElement("Direction"))
                                if (const char *text = directionElement->GetText())
                                    axisPara.Direction = std::string(text) == "true";
                            if (XMLElement *ratioElement = axis->FirstChildElement("Ratio"))
                                if (const char *text = ratioElement->GetText())
                                    axisPara.Ratio = std::stod(text);
                            if (XMLElement *backlashElement = axis->FirstChildElement("Backlash"))
                                if (const char *text = backlashElement->GetText())
                                    axisPara.Backlash = std::stod(text);
                             if (XMLElement *maxPosElement = axis->FirstChildElement("MaxVelocity"))
                                if (const char *text = maxPosElement->GetText())
                                    axisPara.max_vel = std::stod(text);
                            if (XMLElement *minPosElement = axis->FirstChildElement("MaxAcceleration"))
                                if (const char *text = minPosElement->GetText())
                                    axisPara.max_acc= std::stod(text);
                                                
                            if (XMLElement *maxAccElement = axis->FirstChildElement("Encoder"))
                                if (const char *text = maxAccElement->GetText())
                                    axisPara.pos_factor = std::stod(text);                        
                            if (XMLElement *maxPosFollowingErrorElement = axis->FirstChildElement("Negative"))
                                if (const char *text = maxPosFollowingErrorElement->GetText())
                                    axisPara.min_pos = std::stod(text);
                            if (XMLElement *maxVelFollowingErrorElement = axis->FirstChildElement("Positive"))
                                if (const char *text = maxVelFollowingErrorElement->GetText())
                                    axisPara.max_pos = std::stod(text);
                            axisParas.push_back(axisPara);
                        }
                    }
                    else 
                    {
                        throw std::runtime_error("root element axis error");
                    }
                }
                else
                {
                    throw std::runtime_error("load axis.xml file error");
                }
            }
    };
}
#endif
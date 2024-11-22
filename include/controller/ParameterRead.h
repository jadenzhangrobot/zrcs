#ifndef PARAMETERREAD_H
#define PARAMETERREAD_H
#include <vector>
#include <cstdint>
#include <filesystem>
#include "tinyxml2.h"
namespace HWAL 
{
     using namespace tinyxml2;
     inline  std::string GetProjectPath(void)
    {
            std::string currentExePath = std::filesystem::current_path().string();
            std::string target = "zrcs-dev";
            std::string projectPath ;
        // 查找目标字符串 "zrcs" 在 fullPath 中的位置
            size_t found = currentExePath.find(target);
        
            if (found != std::string::npos)
            {
            // 截取 "zrcs" 前面的子串
            projectPath  = currentExePath.substr(0, found);
            }
            else {
                throw std::runtime_error("没有找到工程名 zrcs");
            }
            return projectPath;
    }
    typedef struct
    {
       uint16_t id;
       int64_t encoderBits;
       double  negativeLimit;
       double  positiveLimit;
       double  PositionOffset;
    }MotoPara;
    class MotorConfig
    { 
            tinyxml2::XMLDocument doc;
            public:
            std::vector<MotoPara>motoParas;          
            MotorConfig()
            {
                std::string motorConfigPath=GetProjectPath()+"zrcs-dev/config/motor.xml";
                if (doc.LoadFile(motorConfigPath.c_str())==tinyxml2::XML_SUCCESS) 
                {

                    XMLElement *root = doc.FirstChildElement("MotorConfig");
                    if (root)
                    {       
                    // 遍历子元素,获取所有从站信息
                        for (XMLElement *motor = root->FirstChildElement("motor");motor; motor = motor->NextSiblingElement("motor"))
                        {
                            MotoPara motoPara;
                            const XMLAttribute *idAttr = motor->FindAttribute("ID");
                                if (idAttr) 
                                {
                                    const char *Value = idAttr->Value();           
                                    motoPara.id = std::stoll(Value, nullptr);
                                }
                                const XMLAttribute *EncoderAttr = motor->FindAttribute("Encoder");
                                if (EncoderAttr)
                                {
                                    const char *Value = EncoderAttr->Value();           
                                    motoPara.encoderBits = std::stoll(Value, nullptr);
                                }
                                const XMLAttribute *NegativeLimitAttr = motor->FindAttribute("NegativeLimit");                             
                                if (NegativeLimitAttr) 
                                {
                                    const char *Value = NegativeLimitAttr->Value();           
                                    motoPara.negativeLimit = std::stoll(Value, nullptr);
                                }
                                const XMLAttribute *PositiveLimitAttr = motor->FindAttribute("PositiveLimit");                             
                                if (PositiveLimitAttr) 
                                {
                                    const char *Value = PositiveLimitAttr->Value();           
                                    motoPara.positiveLimit = std::stoll(Value, nullptr);
                                }

                                 const XMLAttribute *PositionOffset = motor->FindAttribute("PositionOffset");                             
                                if (PositionOffset) 
                                {
                                    const char *Value = PositionOffset->Value();           
                                    motoPara.PositionOffset = std::stoll(Value, nullptr);
                                }
                                motoParas.push_back(motoPara);                              
                        }
                    }
                    else 
                    {
                        throw std::runtime_error(" root element motor error");
                    }
                }
                else
                {
                    throw std::runtime_error("load motor.xml file error");
                }

           }
   
    };
}
#endif
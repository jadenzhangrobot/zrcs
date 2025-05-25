#ifndef PARAMETERREAD_H
#define PARAMETERREAD_H
#include "common/xmlParsing.h"
#include "global.h"
#include <string>
#include <vector>
using namespace tinyxml2;
namespace ZrcsHardware 
{ 

    class AxisConfig:public XmlParsing
    { 
            std::vector<AxisPara> axisParas;
            AxisConfig(const std::string& xmlFileName):XmlParsing(xmlFileName)
            {
                for (auto child:tree->root->children)
                {
                     AxisPara axisPara;
                    // axisPara.name = child.second->attribute["name"];
                                     
                }
                          
            }       
    };
}
#endif
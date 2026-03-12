#ifndef MODEL_CONFIG_H
#define MODEL_CONFIG_H
#include "config/parameter.h"
#include "xml/xmlParsing.h"
class ModelConfig:private XmlParsing
{
public:
    ModelConfig(const std::string& xmlFileName):XmlParsing(xmlFileName)
    {
        for (auto child:tree->root->children)
        {
            
        }
    }
};
#endif
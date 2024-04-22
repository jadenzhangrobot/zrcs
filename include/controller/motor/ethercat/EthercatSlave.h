#ifndef ETHERCATSLAVEPDO_H
#define ETHERCATSLAVEPDO_H
#include<iostream>
#include "tinyxml2.h"

namespace controller {
using namespace tinyxml2;
class EthercatSlves
{
     tinyxml2::XMLDocument doc;
     public:
     EthercatSlves()
     {

     }

     int init (void)
     {
        if (doc.LoadFile("../config/ethercat.xml") == tinyxml2::XML_SUCCESS) {
            
                tinyxml2::XMLElement * root= doc.FirstChildElement("Ethercat");
                if (root) {
            // 遍历子元素
            for (XMLElement* slaveelem = root->FirstChildElement("slave"); slaveelem; slaveelem = slaveelem->NextSiblingElement("slave")) {
               const XMLAttribute* idAttr = slaveelem->FindAttribute("ID");
                if (idAttr) {
                    const char* idValue = idAttr->Value();
                    std::cout << "Slave ID: " << idValue << std::endl;

                    // 在这里处理其他的 <slave> 元素内容
                }
            }
        } else {
            std::cerr << "Error: root element not found!" << std::endl;
            return 1;
        }

      
       } else {
    
         return -1;
       }
       return 1;
     }
};
}
#endif
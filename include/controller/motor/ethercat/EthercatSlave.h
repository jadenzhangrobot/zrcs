#ifndef ETHERCATSLAVEPDO_H
#define ETHERCATSLAVEPDO_H
#include <csignal>
#include <cstdint>
#include <cstring>
#include<iostream>
#include "tinyxml2.h"
#include <string>
#include <vector>
#include "ecrt.h"

namespace controller {
using namespace tinyxml2;
class EthercatSlves
{
     tinyxml2::XMLDocument doc;
     public:

   
     struct SlavesInfo{
     std::string SlaveName;
     uint16_t SlaveNum;
     bool ConfigPdo;
     uint32_t VID;
     uint32_t PID;
     uint16_t assignActivate;
     uint32_t sync0Cycle;
     uint32_t sync0Shift;
     uint16_t SmInput;
     uint16_t SmOutput;
     std::vector<ec_pdo_entry_info_t> SlavePdoInput;
     std::vector<ec_pdo_entry_info_t> SlavePdoOutput;
     std::vector<ec_pdo_info_t>SlavePdo;
     };
     std::vector<SlavesInfo>SlavesInfos;
    
     EthercatSlves()
     {

     }

     int init (void)
     {
       
        if (doc.LoadFile("/home/hg/work/zrcs/config/ethercat.xml") == tinyxml2::XML_SUCCESS) {
            
                XMLElement * root= doc.FirstChildElement("Ethercat");
                if (root) {
            // 遍历子元素
            for (XMLElement* slaveelem = root->FirstChildElement("slave"); slaveelem; slaveelem = slaveelem->NextSiblingElement("slave")) {
                //获取从站的id号
                SlavesInfo si;
                si.SlavePdo.resize(2);
                const XMLAttribute* idAttr = slaveelem->FindAttribute("ID");
                if (idAttr) {
                    const char* Value = idAttr->Value();
                  
                    si.SlaveNum=std::stoi(Value);
                }
                //获取从站的vid
                const XMLAttribute* VidAttr = slaveelem->FindAttribute("VID");
                if (VidAttr) 
                {
                      const char* Value = VidAttr->Value();
                      
                      si.VID=std::stoi(Value, nullptr, 16);
                }
                //获取从站的pid
                const XMLAttribute* pidAttr = slaveelem->FindAttribute("PID");
                if (pidAttr) {
                    const char* Value = pidAttr->Value();
                    
                    si.PID=std::stoi(Value, nullptr, 16);
                }
                //获取是不是配置pdo的变量值
                 const XMLAttribute* configpdo = slaveelem->FindAttribute("configPdos");
                if (configpdo) {
                    const char* pidValue = idAttr->Value();
                    if (strcmp(pidValue ,"ture")) {  
                        si.ConfigPdo=true;                  
                    }
                    else if (strcmp(pidValue ,"false")) {
                        si.ConfigPdo=false;
                    }
                    else {
                       std::cout<<"configPdos config error"<<std::endl;
                    }
                }
                //获取从站的名字
                const XMLAttribute* nameAttr = slaveelem->FindAttribute("name");
                if (nameAttr) {
                    const char* Value = nameAttr->Value();
                    si.SlaveName=Value;
                }
                //获取dc的配置参数
                const XMLAttribute* assignActivateAttr = slaveelem->FindAttribute("assignActivate");
                if (assignActivateAttr) {
                    const char* Value = assignActivateAttr->Value();                   
                    si.assignActivate=std::stoi(Value, nullptr, 16);
                }
                 //获取从站的周期
                const XMLAttribute* cycle0Attr = slaveelem->FindAttribute("sync0Cycle");
                if (cycle0Attr) {
                    const char* Value = cycle0Attr->Value();

                    si.sync0Cycle=std::stoi(Value);
                }
                //获取dc的偏移值
                const XMLAttribute* cycle0ShiftAttr = slaveelem->FindAttribute("sync0Shift");
                if (cycle0ShiftAttr) {
                    const char* Value = cycle0ShiftAttr->Value();                   
                    si.sync0Shift=std::stoi(Value);
                }
                
                for (XMLElement* syncManager = slaveelem->FirstChildElement("syncManager"); syncManager; syncManager = syncManager->NextSiblingElement("syncManager")) {

                      const XMLAttribute* SmId= syncManager->FindAttribute("idx");
                      const char* SmIdvalue=SmId->Value();
                       for (XMLElement* pdo = syncManager->FirstChildElement("pdo"); pdo; pdo = pdo->NextSiblingElement("pdo")) {
                               if (std::stoi(SmIdvalue)==2) {
                                    const XMLAttribute* pdoidxAttr = pdo->FindAttribute("idx");
                                    if (pdoidxAttr) {
                                        const char* Value = pdoidxAttr->Value();
                                         si.SlavePdo[0].index=std::stoi(Value, nullptr, 16);
                                    }
                               }
                               else if (std::stoi(SmIdvalue)==3) {
                                    const XMLAttribute* pdoidxAttr = pdo->FindAttribute("idx");
                                    if (pdoidxAttr) {
                                        const char* Value = pdoidxAttr->Value();
                                         si.SlavePdo[1].index=std::stoi(Value, nullptr, 16);
                                    }
                               }
                             
                            for (XMLElement* pdoEntry = pdo->FirstChildElement("pdoEntry"); pdoEntry; pdoEntry = pdoEntry->NextSiblingElement("pdoEntry")) {
                                    ec_pdo_entry_info_t EcPdo;
                                    const XMLAttribute* idxAttr = pdoEntry->FindAttribute("idx");
                                    if (idxAttr) {
                                        const char* Value = idxAttr->Value();
                                         EcPdo.index=std::stoi(Value, nullptr, 16);
                                    }
                                     const XMLAttribute* subidxAttr = pdoEntry->FindAttribute("subIdx");
                                    if (subidxAttr) {
                                        const char* Value = subidxAttr->Value();
                                        EcPdo.subindex=std::stoi(Value, nullptr, 16);
                                    }
                                     const XMLAttribute* bitlenAttr = pdoEntry->FindAttribute("bitLen");
                                    if (bitlenAttr) {
                                        const char* Value = bitlenAttr->Value();
                                        EcPdo.bit_length=std::stoi(Value, nullptr, 16);
                                    }
                                    const XMLAttribute* SmObject= syncManager->FindAttribute("idx");
                                    const char* Value=SmObject->Value();
                                    if (std::stoi(SmIdvalue)==2) {
                                        si.SmOutput=std::stoi(Value, nullptr, 16);
                                        si.SlavePdoOutput.push_back(EcPdo);
                                    }
                                    else if(std::stoi(SmIdvalue)==3)
                                    {
                                        si.SmInput=std::stoi(Value, nullptr, 16);
                                        si.SlavePdoInput.push_back(EcPdo);
                                    }
                            }
                            si.SlavePdo[0].n_entries=si.SlavePdoOutput.size();
                            si.SlavePdo[0].entries=si.SlavePdoOutput.data();
                            si.SlavePdo[1].n_entries=si.SlavePdoInput.size();
                            si.SlavePdo[1].entries=si.SlavePdoInput.data();
                       }


                }
                         
                          
                          SlavesInfos.push_back(si);

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
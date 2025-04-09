#ifndef SLAVECONFIG_H
#define SLAVECONFIG_H
#include "ecrt.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <filesystem>
#include "tinyxml2.h"
namespace ZrcsHardware
{
using namespace tinyxml2;
class SlaveConfig {
tinyxml2::XMLDocument doc;
public:
 std::vector<ec_pdo_entry_info_t>* entries;  
 std::vector<ec_pdo_info_t>* pdos;
 enum SlaveType 
 {
    MOTOR,
    AIO,
    DIO,
 };
  using Slave = struct 
  {
    std::string SlaveName;
    uint16_t SlaveId;
    SlaveType slaveType;
    uint32_t VID;
    uint32_t PID;
    bool ConfigPdo;
    uint16_t AssignActivate;
    uint32_t Sync0Cycle;
    uint32_t Sync0Shift;
    std::vector<ec_sync_info_t> EcSms;
    std::map<std::string,std::string> IndexAndRegister;
  };
  std::vector<Slave> Slaves;

  SlaveConfig(const std::string& projectPath)
  {
        std::string ethercatConfigPath=projectPath+"config/ethercat.xml";
        
        if (doc.LoadFile(ethercatConfigPath.c_str())==tinyxml2::XML_SUCCESS) 
        {

           XMLElement *root = doc.FirstChildElement("Ethercat");
           if (root) {       
        // 遍历子元素,获取所有从站信息
          for (XMLElement *slaveelem = root->FirstChildElement("slave");
             slaveelem; slaveelem = slaveelem->NextSiblingElement("slave"))
           {
           Slave slave_;
          // 获取从站的id号
          const XMLAttribute *idAttr = slaveelem->FindAttribute("ID");
          if (idAttr) {
            const char *Value = idAttr->Value();
            int id = std::stoi(Value);
            slave_.SlaveId = id;
          }
          // 获取从站的vid
          const XMLAttribute *VidAttr = slaveelem->FindAttribute("VID");
          if (VidAttr) {
            const char *Value = VidAttr->Value();           
            slave_.VID = std::stoll(Value, nullptr, 16);
          }
          // 获取从站的pid
          const XMLAttribute *pidAttr = slaveelem->FindAttribute("PID");
          if (pidAttr) {
            const char *Value = pidAttr->Value();

            slave_.PID = std::stoll(Value, nullptr, 16);
          }
          // 获取是不是配置pdo的变量值
          const XMLAttribute *configpdo =
              slaveelem->FindAttribute("configPdos");
          if (configpdo) {
            const char *pidValue = idAttr->Value();
            if (strcmp(pidValue, "ture")) {
              slave_.ConfigPdo = true;
            } else if (strcmp(pidValue, "false")) {
              slave_.ConfigPdo = false;
            } else {
              std::cout << "configPdos config error" << std::endl;
            }
          }
          // 获取从站的名字
          const XMLAttribute *typeAttr = slaveelem->FindAttribute("type");
          if (typeAttr) {
            const char *Value = typeAttr->Value();           
             if (strcmp(Value, "motor")==0) {
                slave_.slaveType = SlaveType::MOTOR;
            } else if (strcmp(Value, "aio")==0){
             slave_.slaveType = SlaveType::AIO;
            } else if ((strcmp(Value, "dio")==0))
            {
               slave_.slaveType= SlaveType::DIO;
            }
             else
            {
              std::cout << "Add the correct slave type" << std::endl;
            }
          }
          // 获取dc的配置参数
          const XMLAttribute *assignActivateAttr =
              slaveelem->FindAttribute("assignActivate");
          if (assignActivateAttr) {
            const char *Value = assignActivateAttr->Value();
            slave_.AssignActivate = std::stoi(Value, nullptr, 16);
          }
          // 获取从站的周期
          const XMLAttribute *cycle0Attr =
              slaveelem->FindAttribute("sync0Cycle");
          if (cycle0Attr) {
            const char *Value = cycle0Attr->Value();

            slave_.Sync0Cycle = std::stoi(Value);
          }
          // 获取dc的偏移值
          const XMLAttribute *cycle0ShiftAttr =
              slaveelem->FindAttribute("sync0Shift");
          if (cycle0ShiftAttr) {
            const char *Value = cycle0ShiftAttr->Value();
            slave_.Sync0Shift = std::stoi(Value);
          }

          for (XMLElement *syncManager =
               slaveelem->FirstChildElement("syncManager");
               syncManager;
               syncManager = syncManager->NextSiblingElement("syncManager")) {
            ec_sync_info_t Ecsm_;
            const XMLAttribute *Smidx = syncManager->FindAttribute("idx");
            if (Smidx) {
              const char *SmIdvalue = Smidx->Value();
              Ecsm_.index = std::stoi(SmIdvalue);
            }
             const XMLAttribute *Smdir = syncManager->FindAttribute("dir");
            if (Smdir) {
              const char *Smdirvalue = Smdir->Value();

              Ecsm_.dir =static_cast<ec_direction_t>(std::stoi(Smdirvalue));
            }
              const XMLAttribute *Smdwatchdog = syncManager->FindAttribute("watchDog");
            if (Smdwatchdog) {
              const char *Smwatchdogvalue = Smdwatchdog->Value();

              Ecsm_.watchdog_mode =static_cast<ec_watchdog_mode_t>(std::stoi(Smwatchdogvalue));
            }
            pdos=new std::vector<ec_pdo_info_t>;
            for (XMLElement *pdo = syncManager->FirstChildElement("pdo"); pdo;
                 pdo = pdo->NextSiblingElement("pdo")) {
              
              ec_pdo_info_t pdo_;
              const XMLAttribute *Pdoindex = pdo->FindAttribute("idx");
              if (Pdoindex) {
                const char *Value = Pdoindex->Value();
                pdo_.index= std::stoi(Value, nullptr, 16);
              }
              entries  =new std::vector<ec_pdo_entry_info_t>;
              for (XMLElement *pdoEntry = pdo->FirstChildElement("pdoEntry");
                   pdoEntry;
                   pdoEntry = pdoEntry->NextSiblingElement("pdoEntry")) {
                ec_pdo_entry_info_t ec_pdo_entry_info_t_;
                const XMLAttribute *idxAttr = pdoEntry->FindAttribute("idx");
                if (idxAttr)
                {
                  const char *Value = idxAttr->Value();
                  ec_pdo_entry_info_t_.index = std::stoi(Value, nullptr, 16);
                }
                const XMLAttribute *subidxAttr =pdoEntry->FindAttribute("subIdx");
                if (subidxAttr)
                {
                  const char *Value = subidxAttr->Value();
                  ec_pdo_entry_info_t_.subindex = std::stoi(Value, nullptr, 16);
                }
                const XMLAttribute *bitlenAttr =
                    pdoEntry->FindAttribute("bitLen");
                if (bitlenAttr)
                 {
                  const char *Value = bitlenAttr->Value();
                  ec_pdo_entry_info_t_.bit_length = std::stoi(Value);
                 }

                 const XMLAttribute *registerAttr =
                    pdoEntry->FindAttribute("name");
                if (registerAttr)
                 {
                  const char *Value = registerAttr->Value();
                  std::string objectDictionary=std::to_string(ec_pdo_entry_info_t_.index)+std::to_string(ec_pdo_entry_info_t_.subindex);
                  slave_.IndexAndRegister.insert(std::pair<std::string, std::string>(objectDictionary,Value));
                 }
                
                entries->push_back(ec_pdo_entry_info_t_);
              
              }
              pdo_.n_entries=entries->size();
              pdo_.entries=entries->data();
              pdos->push_back(pdo_);
            }
            Ecsm_.n_pdos=pdos->size();
            if (pdos->size()==0) {
                Ecsm_.pdos=nullptr;
            }
            else {
                Ecsm_.pdos=pdos->data();
            }            
            slave_.EcSms.push_back(Ecsm_);
          }
        ec_sync_info_t  esit;
        esit.index=0xff;
        slave_.EcSms.push_back(esit);
        Slaves.push_back(slave_);
        }
       
      }
    } else{   
          throw std::runtime_error("Error: root element not found!");
    }

  }
  ~SlaveConfig()
  {     
        delete entries;
        delete pdos;
  }
};
} // namespace controller
#endif
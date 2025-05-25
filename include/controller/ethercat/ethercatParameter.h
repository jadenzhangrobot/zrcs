#ifndef SLAVECONFIG_H
#define SLAVECONFIG_H
#include "ecrt.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include "common/xmlParsing.h"
namespace ZrcsHardware
{
class SlaveConfig {
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

  SlaveConfig()
  {
        
          XmlParsing xmlParsing("ethercat.xml");

          for (auto child : xmlParsing.getRootNode()->children)
          {
             Slave slave;
             slave.SlaveId= std::stoi(child.second->attribute["ID"] );
             slave.VID= std::stoll(child.second->attribute["VID"], nullptr, 16);
             slave.PID= std::stoll(child.second->attribute["PID"], nullptr, 16);
             slave.ConfigPdo=child.second->attribute["configPdos"]=="true"?true:false;
            if (child.second->attribute["type"]=="motor")
            {
                slave.slaveType=SlaveType::MOTOR;
            }
            else if(child.second->attribute["type"]=="aio")
            { 
                slave.slaveType=SlaveType::AIO;               
            }
            else if (child.second->attribute["type"]=="dio")
            {
                slave.slaveType=SlaveType::DIO;
            }
            else
            {
                throw std::runtime_error("Add the correct slave type");
            }
            slave.AssignActivate=std::stoi(child.second->attribute["assignActivate"], nullptr, 16);
            slave.Sync0Cycle=std::stoi(child.second->attribute["sync0Cycle"]);
            slave.Sync0Shift=std::stoi(child.second->attribute["sync0Shift"]);
            for (auto syncManager : child.second->children)
            {
                ec_sync_info_t ecsm;
                ecsm.index=std::stoi(syncManager.second->attribute["idx"]);
                ecsm.dir=static_cast<ec_direction_t>(std::stoi(syncManager.second->attribute["dir"]));
                ecsm.watchdog_mode=static_cast<ec_watchdog_mode_t>(std::stoi(syncManager.second->attribute["watchDog"]));
                pdos=new std::vector<ec_pdo_info_t>;
                for (auto pdo : syncManager.second->children)
                {
                    ec_pdo_info_t pdo_;
                    pdo_.index=std::stoi(pdo.second->attribute["idx"], nullptr, 16);
                    
                    entries =new std::vector<ec_pdo_entry_info_t>;
                    for (auto pdoEntry : pdo.second->children)
                    {
                        ec_pdo_entry_info_t pdoEntry_;
                        pdoEntry_.index=std::stoi(pdoEntry.second->attribute["idx"], nullptr, 16);
                        pdoEntry_.subindex=std::stoi(pdoEntry.second->attribute["subIdx"], nullptr, 16);
                        pdoEntry_.bit_length=std::stoi(pdoEntry.second->attribute["bitLen"]);
                        slave.IndexAndRegister.insert(std::make_pair(std::to_string(pdoEntry_.index)+std::to_string(pdoEntry_.subindex),pdoEntry.second->attribute["name"]));
                        entries->push_back(pdoEntry_);
                    }
                    pdo_.n_entries=entries->size();
                    pdo_.entries = entries->data();                
                    pdos->push_back(pdo_);
                }
                ecsm.n_pdos=pdos->size();
                if (pdos->size()==0) {
                    ecsm.pdos=nullptr;
                }
                else {
                    ecsm.pdos=pdos->data();
                }
                slave.EcSms.push_back(ecsm);
            }             
              ec_sync_info_t esit;
              esit.index=0xff;
              slave.EcSms.push_back(esit);
              Slaves.push_back(slave);
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
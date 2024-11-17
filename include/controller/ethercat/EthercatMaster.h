#ifndef ETHERCATMASTER
#define ETHERCATMASTER
#include "ecrt.h"
#include <alchemy/timer.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "EthercatParameterRead.h"
#include "controller/ControllerInterface.h"
namespace HWAL {
#define  SMOUT 2 //ethercat第二个同步管理器
#define  SMIN 3 //ethercat第三个同步管理器
class EthercatMaster {
private:

  static inline ec_master_t *master = NULL;

  static inline ec_master_state_t master_state = {};

  static inline ec_domain_t *DomainInput = NULL;
  static inline ec_domain_t *DomainOutput = NULL;
  static inline ec_domain_state_t domain_state = {};

  static inline ec_slave_config_t *sc;
  static inline ec_slave_config_state_t sc_state = {};

 static inline std::vector<ec_pdo_entry_reg_t> DomainInputReg;
 static inline std::vector<ec_pdo_entry_reg_t> DomainOutputReg;

 
  static inline std::vector<ec_domain_t *> ec_domanin;
  int AllOutputPdoCount=0;
  int AllInputPdoCount=0;
  
 
public:
  SlaveConfig*  slaveConfig;
  static inline uint8_t *DomainWrite = NULL;
  static inline uint8_t *DomainRead= NULL;
  static inline std::vector<std::vector<uint32_t>> OutputOffset;
  static inline std::vector<std::vector<uint32_t>> InputOffset;

  std::vector<std::map<std::string, int>> InputPdoInfoAndOffset;
  std::vector<std::map<std::string, int>> OutputPdoInfoAndOffset;

  EthercatMaster() : slaveConfig(new SlaveConfig()) 
  {
    EthercatInit();
  }
  ~EthercatMaster() {
    ecrt_release_master(master);
    delete slaveConfig;
  }
  
  void EthercatInit() {
     OutputOffset.resize(slaveConfig->Slaves.size());
     InputOffset.resize(slaveConfig->Slaves.size());
     InputPdoInfoAndOffset.resize(slaveConfig->Slaves.size());
     OutputPdoInfoAndOffset.resize(slaveConfig->Slaves.size());

   
    for (int i=0; i<slaveConfig->Slaves.size();i++)
     {
         int OutputPdoCount=0;
         int InputPdoCount=0;
         for ( int j=0;j<slaveConfig->Slaves[i].EcSms[SMOUT].n_pdos;j++)
          {
            for(int k=0;k<slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries;k++)
            {
                 OutputPdoCount++;
            }
          }
         
         for ( int j=0;j<slaveConfig->Slaves[i].EcSms[SMIN].n_pdos;j++) 
         {
            for(int k=0;k<slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries;k++)
            {
              InputPdoCount++;
            }
         }
          AllOutputPdoCount=AllOutputPdoCount+OutputPdoCount;
          AllInputPdoCount=AllInputPdoCount+InputPdoCount;
          OutputOffset[i].resize(OutputPdoCount);    
          InputOffset[i].resize(InputPdoCount);
    }
      DomainOutputReg.resize(AllOutputPdoCount);
      DomainInputReg.resize(AllInputPdoCount);
   
  
    master = ecrt_request_master(0);
    if (master == nullptr) 
    {
       throw std::runtime_error("获取ethercat主站失败");
    }

    DomainInput = ecrt_master_create_domain(master);
    DomainOutput = ecrt_master_create_domain(master);
    if ((DomainInput == nullptr)||(DomainOutput==nullptr))
    {
      throw std::runtime_error("创建domain失败");
    }
    for (int i = 0; i < slaveConfig->Slaves.size(); i++) 
    {
      sc=ecrt_master_slave_config(master, 0, i, slaveConfig->Slaves[i].VID,slaveConfig->Slaves[i].PID);
      if (sc==NULL) 
      {
        throw std::runtime_error("ethercat从站配置失败");
      } 

      if (ecrt_slave_config_pdos(sc, EC_END, slaveConfig->Slaves[i].EcSms.data())<0)
      {
            throw std::runtime_error("ethercat配置pdo失败");
      } 
      if (i == 0) 
      {
         if(ecrt_master_select_reference_clock(master, sc)<0)
         {
             throw std::runtime_error("选择参考时钟失败");
         }
      }
        ecrt_slave_config_dc(sc, slaveConfig->Slaves[i].AssignActivate,slaveConfig->Slaves[i].Sync0Cycle, slaveConfig->Slaves[i].Sync0Shift, 0, 0);
    
          
        for ( int j=0;j<slaveConfig->Slaves[i].EcSms[SMOUT].n_pdos;j++)
        {
            for(int k=0;k<slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries;k++)
            {
              uint16_t count=i*slaveConfig->Slaves[i].EcSms[SMOUT].n_pdos*slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries+j*slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries+k;
              uint16_t offset=j*slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].n_entries+k;
              DomainOutputReg[count].alias=0;
              DomainOutputReg[count].position=i;
              DomainOutputReg[count].vendor_id=slaveConfig->Slaves[i].VID;
              DomainOutputReg[count].product_code=slaveConfig->Slaves[i].PID;
              DomainOutputReg[count].index=slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].entries[k].index;
              DomainOutputReg[count].subindex=slaveConfig->Slaves[i].EcSms[SMOUT].pdos[j].entries[k].subindex;          
              DomainOutputReg[count].offset=&OutputOffset[i][offset];
              DomainOutputReg[count].bit_position=nullptr;
              auto it=  slaveConfig->Slaves[i].IndexAndRegister.find(std::to_string(DomainOutputReg[count].index)+std::to_string(DomainOutputReg[count].subindex));
              // 检查是否找到并输出值
              if (it != slaveConfig->Slaves[i].IndexAndRegister.end()) 
              {
                    OutputPdoInfoAndOffset[i].emplace(it->second,offset);   
              } 
              else
              {
                    throw std::runtime_error("没有找到对应的寄存器");
              }     
             
            }
        }
             
         for ( int j=0;j<slaveConfig->Slaves[i].EcSms[SMIN].n_pdos;j++) 
         {
            for(int k=0;k<slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries;k++)
            {
              uint16_t count=i*slaveConfig->Slaves[i].EcSms[SMIN].n_pdos*slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries+j*slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries+k;
              uint16_t offset=j*slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].n_entries+k;
              DomainInputReg[count].alias=0;
              DomainInputReg[count].position=i;
              DomainInputReg[count].vendor_id=slaveConfig->Slaves[i].VID;
              DomainInputReg[count].product_code=slaveConfig->Slaves[i].PID;
              DomainInputReg[count].index=slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].entries[k].index;
              DomainInputReg[count].subindex=slaveConfig->Slaves[i].EcSms[SMIN].pdos[j].entries[k].subindex;            
              DomainInputReg[count].offset=&InputOffset[i][offset];
              DomainInputReg[count].bit_position=nullptr;
              auto it=  slaveConfig->Slaves[i].IndexAndRegister.find(std::to_string(DomainInputReg[count].index)+std::to_string(DomainInputReg[count].subindex));
              // 检查是否找到并输出值
              if (it != slaveConfig->Slaves[i].IndexAndRegister.end()) 
              {
                    InputPdoInfoAndOffset[i].emplace(it->second,offset);   
              } 
              else
              {
                    throw std::runtime_error("没有找到对应的寄存器");
              }          
            }
         }            
    }
    if (ecrt_domain_reg_pdo_entry_list(DomainOutput, DomainOutputReg.data())||ecrt_domain_reg_pdo_entry_list(DomainInput, DomainInputReg.data()))
    {
        throw std::runtime_error("映射domain内存失败");
    }
    if (ecrt_master_activate(master) < 0) 
    {
        throw std::runtime_error("激活ethercat主站失败");
    }

    if ((DomainWrite = ecrt_domain_data(DomainOutput)) == nullptr)
     {
         throw std::runtime_error("获取domain的写入地址失败");
    }
    if ((DomainRead = ecrt_domain_data(DomainInput)) == nullptr) {
        throw std::runtime_error("获取domain的读入地址失败");
    }
  }

  void send() 
  {
    ecrt_domain_queue(DomainOutput);
    ecrt_domain_queue(DomainInput);
    ecrt_master_application_time(master, rt_timer_read());
    ecrt_master_sync_reference_clock(master);
    ecrt_master_sync_slave_clocks(master);
    ecrt_master_send(master);
  }

  void receive()
  {
    ecrt_master_receive(master);
    ecrt_domain_process(DomainOutput);
    ecrt_domain_process(DomainInput);
  }
};
} 
#endif

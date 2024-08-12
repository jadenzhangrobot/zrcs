#ifndef ETHERCATMASTER
#define ETHERCATMASTER
#include "EthercatSlave.h"
#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <cstdint>
#include <iostream>
#include <map>
#include <rtdm/rtdm.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>
#define  SM2 2 //ethercat第二个同步管理器
#define  SM3 3 //ethercat第三个同步管理器
namespace controller {
// using PdOInfo =struct
// {
//     int  SlaveId;
//     std::uint16_t index;
//     std::uint8_t subinx;
// };
class EthercatMaster {
private:
  EthercatSlves *ecslave;

  static inline uint cycle_ns = 1000000;

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
  int OutputPdoCount=0;
  int InputPdoCount=0;

public:
  static inline std::vector<uint32_t> OutputOffset;
  static inline std::vector<uint32_t> InputOffset;
  static inline uint8_t *DomainWrite = NULL;
  static inline uint8_t *DomainRead= NULL;
  std::map<std::string, int> InputPdoInfoAndOffset;
  std::map<std::string, int> OutputPdoInfoAndOffset;

  EthercatMaster() : ecslave(new EthercatSlves) {
    EthercatInit();
  }
  ~EthercatMaster() {
    ecrt_release_master(master);
    delete ecslave;
  }
  static EthercatMaster &getInstance(void) {
    static EthercatMaster em;
    return em;
  }
  int EthercatInit() {
    for (int i=0; i<ecslave->Slaves.size();i++) {
       
      for ( int j=0;j<ecslave->Slaves[i].EcSms[SM2].n_pdos;j++)
        {
          for(int k=0;k<ecslave->Slaves[i].EcSms[SM2].pdos[j].n_entries;k++)
          {
              OutputPdoCount++;
          }
        }

         for ( int j=0;j<ecslave->Slaves[i].EcSms[SM3].n_pdos;j++) 
         {
          for(int k=0;k<ecslave->Slaves[i].EcSms[SM3].pdos[j].n_entries;k++)
          {
            InputPdoCount++;
          }
         }
    }
    DomainOutputReg.resize(OutputPdoCount);
    OutputOffset.resize(OutputPdoCount);
    DomainInputReg.resize(InputPdoCount);
    InputOffset.resize(InputPdoCount);
    master = ecrt_request_master(0);

    if (master == nullptr) {
      std::cout << "获取主站失败" << std::endl;
      return -1;
    }

    DomainInput = ecrt_master_create_domain(master);
    DomainOutput = ecrt_master_create_domain(master);
    if ((DomainInput == nullptr)||(DomainOutput==nullptr)) {
      std::cout << "创建domain失败" << std::endl;
      return -1;
    }
    for (int i = 0; i < ecslave->Slaves.size(); i++) {
      if (!(sc = ecrt_master_slave_config(master, 0, i, ecslave->Slaves[i].VID,
                                          ecslave->Slaves[i].PID))) {
        fprintf(stderr, "Failed to get slave configuration for slave!\n");
        return -1;
      } else {
        std::cout << "Configuring PDOs" << std::endl;
      }

      if (ecrt_slave_config_pdos(sc, EC_END, ecslave->Slaves[i].EcSms.data()) !=
          0) {
        fprintf(stderr, "Failed to configure slave PDOs!\n");
        return -1;
      } else {
        std::cout << "Success to configuring slave PDOs" << std::endl;
      }
      if (i == 0) {
        ecrt_master_select_reference_clock(master, sc);
      }
      ecrt_slave_config_dc(sc, ecslave->Slaves[i].AssignActivate,
                           ecslave->Slaves[i].Sync0Cycle,
                           ecslave->Slaves[i].Sync0Shift, 0, 0);
          
        for ( int j=0;j<ecslave->Slaves[i].EcSms[SM2].n_pdos;j++)
        {
          for(int k=0;k<ecslave->Slaves[i].EcSms[SM2].pdos[j].n_entries;k++)
          {
            static int Count=0;
            DomainOutputReg[Count].alias=0;
            DomainOutputReg[Count].position=i;
            DomainOutputReg[Count].vendor_id=ecslave->Slaves[i].VID;
            DomainOutputReg[Count].product_code=ecslave->Slaves[i].PID;
            DomainOutputReg[Count].index=ecslave->Slaves[i].EcSms[SM2].pdos[j].entries[k].index;
            DomainOutputReg[Count].subindex=ecslave->Slaves[i].EcSms[SM2].pdos[j].entries[k].subindex;            
            DomainOutputReg[Count].offset=&OutputOffset[Count];
            DomainOutputReg[Count].bit_position=nullptr;
            std::string str=std::to_string(i)+std::to_string(ecslave->Slaves[i].EcSms[SM2].pdos[j].entries[k].index)+std::to_string(ecslave->Slaves[i].EcSms[SM2].pdos[j].entries[k].subindex);
            
            OutputPdoInfoAndOffset.insert(std::pair<std::string,int>(str, Count));         
            Count++;
          }
        }
             
         for ( int j=0;j<ecslave->Slaves[i].EcSms[SM3].n_pdos;j++) 
         {
          for(int k=0;k<ecslave->Slaves[i].EcSms[SM3].pdos[j].n_entries;k++)
          {
            static int Count=0;
            DomainInputReg[Count].alias=0;
            DomainInputReg[Count].position=i;
            DomainInputReg[Count].vendor_id=ecslave->Slaves[i].VID;
            DomainInputReg[Count].product_code=ecslave->Slaves[i].PID;
            DomainInputReg[Count].index=ecslave->Slaves[i].EcSms[SM3].pdos[j].entries[k].index;
            DomainInputReg[Count].subindex=ecslave->Slaves[i].EcSms[SM3].pdos[j].entries[k].subindex;            
            DomainInputReg[Count].offset=&InputOffset[Count];
            DomainInputReg[Count].bit_position=nullptr;
            std::string str=   std::to_string(i)+std::to_string(ecslave->Slaves[i].EcSms[SM3].pdos[j].entries[k].index)+std::to_string(ecslave->Slaves[i].EcSms[SM3].pdos[j].entries[k].subindex);
            
            InputPdoInfoAndOffset.insert(std::pair<std::string,int>(str, Count));
            Count++;          
          }
         }        
    
    }

    if (ecrt_domain_reg_pdo_entry_list(DomainOutput, DomainOutputReg.data())||ecrt_domain_reg_pdo_entry_list(DomainInput, DomainInputReg.data()))
    {
        fprintf(stderr, "PDO entry registration failed!\n");
        return -1;
    }
    else {
        std::cout<<"PDO entry registration"<<std::endl;
    }

    if (ecrt_master_activate(master) < 0) {
      return -1;
    }

    if ((DomainWrite = ecrt_domain_data(DomainOutput)) == nullptr) {
      fprintf(stderr, "PDO entry registration failed!\n");
      return -1;
    }
    if ((DomainRead = ecrt_domain_data(DomainInput)) == nullptr) {
       fprintf(stderr, "PDO entry registration failed!\n");
      return -1;
    }
    return 1;
  }

  void SendData() {
    ecrt_domain_queue(DomainOutput);
    ecrt_domain_queue(DomainInput);
    ecrt_master_application_time(master, rt_timer_read());
    ecrt_master_sync_reference_clock(master);
    ecrt_master_sync_slave_clocks(master);
    ecrt_master_send(master);
  }

  void ReceiveData() {
    ecrt_master_receive(master);
    ecrt_domain_process(DomainOutput);
    ecrt_domain_process(DomainInput);
  }
};
} // namespace controller
#endif

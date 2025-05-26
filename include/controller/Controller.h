#ifndef CONTROLLER
#define CONTROLLER
#include <memory>
#include <vector>
#include "axisParameter.h"
#include "ControllerInterface.h"
#ifdef REALTIME
#include "controller/rtos/xenomai.h"
#endif
#include "ethercat/EthercatMaster.h"
#include "ethercat/EthercatMotor.h"
#include "controller/rtos/linux.h"
#
namespace ZrcsHardware {
   
 class Controller
    {  
        private:     
         EthercatMaster* ethercatMaster;    
        // ParaConfig *axConfig;
        public:
        Controller():axConfig(new ParaConfig("axis.xml")),ethercatMaster(new EthercatMaster())
        {
           
            for(auto it=axConfig->axisParas.begin();it!=axConfig->axisParas.end();++it)
            {                
                    std::unique_ptr<ZrcsHardware::Axis> axis((ZrcsHardware::Axis*)(new EthercatMotor(it->id,ethercatMaster,axConfig)));
                    axiss.push_back(std::move(axis));             
            }
            #ifdef REALTIME
              rtos_.reset((ZrcsHardware::Rtos*)(new xenomai()));
            #else
             rtos_.reset((ZrcsHardware::Rtos*)(new Nativelinux()));
             #endif

                if (!ethercatMaster->OutputOffset.empty()) {
                   outputData.resize( ethercatMaster->OutputOffset.back().back());
                   inputData.resize( ethercatMaster->InputOffset.back().back());                   
                } else {
                    // 处理空向量的情况（如抛出异常或返回错误）
                }
                     

        }
            
         void SendData()
         {
              
                    std::memcpy(outputData.data(), ethercatMaster->DomainWrite, outputData.size());
                    ethercatMaster->send();
                
         }
         void receiveData()
         { 
              
                    ethercatMaster->receive();
                    std::memcpy(inputData.data(),ethercatMaster->DomainRead, inputData.size());
               

         }
         void readIo()
         {
              
         }
         void writeIo()
         {
            
         } 
        ~Controller()
        {
               delete ethercatMaster;
               delete axConfig;
              
        }
        std::vector<uint8_t> outputData;
        std::vector<uint8_t> inputData;
        std::shared_ptr<Rtos> rtos_;
        std::vector<std::unique_ptr<Axis>> axiss;
        std::vector<std::unique_ptr<Io>> Ios;
    };
}
#endif
#ifndef CONTROLLER
#define CONTROLLER
#include <memory>
#include <vector>
#include "axisParameter.h"
#include "ControllerInterface.h"
#ifdef REALTIME
#include "controller/rtos/xenomai.h"
#include "ethercat/EthercatMaster.h"
#include "ethercat/EthercatMotor.h"
#include "ethercat/EthercatIo.h"
#endif
namespace ZrcsHardware {
   
 class Controller
    {  
        private:
        #ifdef REALTIME
          EthercatMaster* ethercatMaster;
        #endif
         ParaConfig *axConfig;
        public:
        Controller()
        {  
            #ifdef REALTIME
            for(auto it=axConfig->axisParas.begin();it!=axConfig->axisParas.end();++it)
            {                
                    std::unique_ptr<ZrcsHardware::Axis> axis((ZrcsHardware::Axis*)(new EthercatMotor(it->slaveid,ethercatMaster)));
                    axiss.push_back(std::move(axis));             
            }
            rtos_.reset((ZrcsHardware::Rtos*)(new xenomai()));
                if (!ethercatMaster->OutputOffset.empty()) {
                   outputData.resize( ethercatMaster->OutputOffset.back().back());
                   inputData.resize( ethercatMaster->InputOffset.back().back());                   
                } else {
                    // 处理空向量的情况（如抛出异常或返回错误）
                }
            #endif         

        }
            
         void SendData()
         {
               #ifdef REALTIME
                    std::memcpy(outputData.data(), ethercatMaster->DomainWrite, outputData.size());
                    ethercatMaster->send();
                #endif
         }
         void receiveData()
         { 
               #ifdef REALTIME
                    ethercatMaster->receive();
                    std::memcpy(inputData.data(),ethercatMaster->DomainRead, inputData.size());
               #endif

         }
         void readIo()
         {
              
         }
         void writeIo()
         {
            
         } 
        ~Controller()
        {
             //  delete ethercatMaster;
              
        }
        std::vector<uint8_t> outputData;
        std::vector<uint8_t> inputData;
        std::shared_ptr<Rtos> rtos_;
        std::vector<std::unique_ptr<Axis>> axiss;
        std::vector<std::unique_ptr<Io>> Ios;
    };
}
#endif
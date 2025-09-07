#ifndef CONTROLLER
#define CONTROLLER
#include <memory>
#include <vector>
#include "axisConfig.h"
#include "ControllerInterface.h"
#include "controller/ethercat/EthercatMaster.h"
#ifdef REALTIME 
#include "controller/rtos/xenomai.h"
#include "ethercat/EthercatMaster.h"
#include "ethercat/EthercatMotor.h"
#endif
#include <controller/virtual/virtualServo.h>
#include <controller/virtual/Coppeliasim.h>

#include "controller/rtos/linux.h"
#
namespace ZrcsHardware {
   
 class Controller
    {  
        private:     
        // EthercatMaster* ethercatMaster;    
         AxisConfig *axConfig;
         EthercatMaster *ethercatMaster;
        public:
        Controller():axConfig(new AxisConfig("axisConfig.xml")), ethercatMaster(new EthercatMaster())
        {
           
           
            #ifdef REALTIME
                  for(auto it=axConfig->axisParas.begin();it!=axConfig->axisParas.end();++it)
                  {                
                        axiss.push_back(new Axis(it->axisId,it->slaveId,&*it,new EthercatMotor(it->slaveId,ethercatMaster)));             
                  }
                  rtos_.reset((ZrcsHardware::Rtos*)(new xenomai()));
               //     if (!ethercatMaster->OutputOffset.empty()) {
               //     outputData.resize( ethercatMaster->OutputOffset.back().back());
               //     inputData.resize( ethercatMaster->InputOffset.back().back());                   
               //  } else {
               //      // 处理空向量的情况（如抛出异常或返回错误）
               //  }
            #else
                  for(auto it=axConfig->axisParas.begin();it!=axConfig->axisParas.end();++it)
                  {                
                        axiss.push_back(new Axis(it->axisId,it->slaveId,&*it,std::make_unique<Coppeliasim>(it->slaveId)));             
                  }
                  rtos_.reset((ZrcsHardware::Rtos*)(new Nativelinux()));
            #endif

               
                     

        }
            
         void SendData()
         {             
                  for(auto it=axiss.begin();it!=axiss.end();++it)
                  {
                    (*it)->updateMotionCmdsToServo();
                  }
                  ethercatMaster->send();

         }
         void receiveData()
         {       ethercatMaster->receive();
                 for(auto it=axiss.begin();it!=axiss.end();++it)
                 {
                    (*it)->statusSync();
                    (*it)->cyclerun();
                 }

         }
         void readIo()
         {
              
         }
         void writeIo()
         {
            
         } 
        ~Controller()
        {
           delete axConfig;
           delete ethercatMaster;
           for (auto ptr : axiss) 
           {
            delete ptr; // 对每个指针调用 delete
           }
           axiss.clear(); // 清空vector，虽然不是必须，但算是一个好习惯

        // 遍历并删除 Ios 中的所有对象
         for (auto ptr : Ios)
         {
            delete ptr; // 对每个指针调用 delete
         }
         Ios.clear();
              
        }
        std::vector<uint8_t> outputData;
        std::vector<uint8_t> inputData;
        std::shared_ptr<Rtos> rtos_;
        std::vector<Axis*> axiss;
        std::vector<Io*> Ios;
    };
}
#endif
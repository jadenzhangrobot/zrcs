#ifndef CONTROLLER
#define CONTROLLER
#include <fstream>
#include <memory>
#include <vector>
#include "axisConfig.h"
#include "ControllerInterface.h"
#ifdef REALTIME 
#include "ethercat/EthercatMaster.h"
#include "controller/rtos/xenomai.h"
#include "ethercat/EthercatMotor.h"
#endif
#include <controller/virtual/virtualServo.h>
#include <controller/virtual/Coppeliasim.h>
#include "controller/rtos/linux.h"
#include "common/rtLog.h"
#
namespace ZrcsHardware {
   
 class Controller
    {  
        private:        
         AxisConfig *axConfig;
          public:
         #ifdef REALTIME 
          EthercatMaster* ethercatMaster;
          Controller():axConfig(new AxisConfig("axisConfig.xml")),ethercatMaster(new EthercatMaster())
          #else 
          Controller():axConfig(new AxisConfig("axisConfig.xml"))
         #endif
        {
                  
                  for(auto it=axConfig->axisParas.begin();it!=axConfig->axisParas.end();++it)
                  {     
                        if (it->controller == "ethercat") 
                        {
                           #ifdef REALTIME 
                              Axis* axis=new Axis(it->axisId,it->slaveId,&*it); 
                              axis->pushServo(new EthercatMotor(it->slaveId,ethercatMaster));
                              axiss.push_back(axis);                           
                           #endif
                        }
                        else if (it->controller == "coppeliasim")
                        {
                           Axis* axis=new Axis(it->axisId,it->slaveId,&*it); 
                           axis->pushServo(new Coppeliasim(it->slaveId));
                           axiss.push_back(axis);

                        }
                        else if (it->controller == "virtual")
                        {
                          Axis* axis=new Axis(it->axisId,it->slaveId,&*it); 
                          axis->pushServo(new virtualServo(it->slaveId));
                          axiss.push_back(axis);
                        }
                        else 
                        {
                          throw std::runtime_error("未定义的控制器类型");
                        }       
                  }
          
                 
                  #ifdef REALTIME 
                        rtos_.reset((ZrcsHardware::Rtos*)(new xenomai()));            
                  #else
                        rtos_.reset((ZrcsHardware::Rtos*)(new Nativelinux()));
                  #endif                  

        }
            
         void SendData()
         {             
                  for(auto it=axiss.begin();it!=axiss.end();++it)
                  {
                    (*it)->updateMotionCmdsToServo();
                  }
                 #ifdef REALTIME 
                  rtos_->send();
                 #endif

         }
         void receiveData()
         {      
                  #ifdef REALTIME 
                     rtos_->receive();
                  #endif
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
           #ifdef REALTIME 
           delete ethercatMaster;
           #endif
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
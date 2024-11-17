#ifndef CONTROLLER
#define CONTROLLER
#include <cstdint>
#include <memory>
#include "ControllerInterface.h"
#include "controller/rtos/xenomai.h"
#include "ethercat/EthercatMaster.h"
#include "ethercat/EthercatMotor.h"
#include "ControllerInterface.h"
#include "ethercat/EthercatIo.h"
namespace HWAL {
   
 class Controller
    {  
        private:
        EthercatMaster* ethercatMaster;
        MotorConfig* motorConfig;

        public:
        Controller():ethercatMaster(new EthercatMaster()),motorConfig(new MotorConfig())
        {          
            for(auto it=ethercatMaster->slaveConfig->Slaves.begin();it!=ethercatMaster->slaveConfig->Slaves.end();++it)
            { 
                if (it->slaveType==SlaveConfig::MOTOR)
                {                   
                     std::unique_ptr<HWAL::Motor> motor((HWAL::Motor*)(new EthercatMotor(it->SlaveId,ethercatMaster,motorConfig)));
                     motors.push_back(std::move(motor));
                }
                else if (it->slaveType==SlaveConfig::AIO)
                {
                
                }
                else if (it->slaveType==SlaveConfig::DIO)
                {
                      std::unique_ptr<HWAL::Io> io((HWAL::Io*)(new EthercatIo(it->SlaveId,ethercatMaster)));
                       Ios.push_back(std::move(io));
                }
                else
                {
                     throw std::runtime_error("没有的从站类型");
                }
            }
               rtos_.reset((HWAL::Rtos*)(new xenomai()));

        }
            
         void SendData()
         {
                 ethercatMaster->send();
         }
         void receiveData()
         {
                ethercatMaster->receive();

         }
        ~Controller()
        {
               delete ethercatMaster;
               delete motorConfig;
          
        }
        std::shared_ptr<Rtos> rtos_;
        std::vector<std::unique_ptr<Motor>> motors;
        std::vector<std::unique_ptr<Io>> Ios;
    };
}
#endif
#ifndef CONTROLLER
#define CONTROLLER
#include <cstdint>
#include <memory>
#include <vector>
#include "ControllerInterface.h"
#include "controller/rtos/xenomai.h"
#include "ethercat/EthercatMaster.h"
#include "ethercat/EthercatMotor.h"
#include "ControllerInterface.h"
#include "ethercat/EthercatIo.h"
namespace ZrcsHardware {
   
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
                     std::unique_ptr<ZrcsHardware::Motor> motor((ZrcsHardware::Motor*)(new EthercatMotor(it->SlaveId,ethercatMaster,motorConfig)));
                     motors.push_back(std::move(motor));
                }
                else if (it->slaveType==SlaveConfig::AIO)
                {
                
                }
                else if (it->slaveType==SlaveConfig::DIO)
                {
                      std::unique_ptr<ZrcsHardware::Io> io((ZrcsHardware::Io*)(new EthercatIo(it->SlaveId,ethercatMaster)));
                      Ios.push_back(std::move(io));
                }
                else
                {
                     throw std::runtime_error("没有的从站类型");
                }
            }
                rtos_.reset((ZrcsHardware::Rtos*)(new xenomai()));
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
               delete motorConfig;
          
        }
        std::vector<uint8_t> outputData;
        std::vector<uint8_t> inputData;
        std::shared_ptr<Rtos> rtos_;
        std::vector<std::unique_ptr<Motor>> motors;
        std::vector<std::unique_ptr<Io>> Ios;
    };
}
#endif
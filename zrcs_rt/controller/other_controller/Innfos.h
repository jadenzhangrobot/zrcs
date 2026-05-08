/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-28 10:52:49
 * @LastEditTime: 2023-06-06 16:45:42
 * @Description: innfos机器人控制器
 * 
 */
#pragma once

//#include "controller/controller_interface.h"
#include "controller/controller_interface.h"
#include "system/log/RtLog.h"
#include "../lib/innfos/include/actuatorcontroller.h"
#include <boost/iterator/iterator_concepts.hpp>


  namespace controller
  {
      class Innfos
      {
          public: 
          std::shared_ptr<ActuatorController> pController;
          Actuator::ErrorsDefine ec;
        
          Innfos()
          {
           pController.reset(ActuatorController::initController());
            
           pController->lookupActuators(ec);
           

          }
          static Innfos& getInstance()
          {
              static Innfos innfos_instance;
              return innfos_instance;
          }
      };
    class InnfosMotor : public Motor
    {
        int motor_id;
        const std::string ip_addr="192.168.1.30";
        Innfos& If= Innfos::getInstance();
        Actuator::ErrorsDefine ec;
       
      public:
        InnfosMotor(int nm):motor_id(nm)
        {
        }
        ~InnfosMotor()
        {
        }

        auto  setTargetPos(double pos)->int override
        { 
              If.pController->setPosition(motor_id+1,pos,ip_addr);
              return 1;
        }
        auto  actualPos()->double override
        {
            double pos=If.pController->getPosition(motor_id+1,true,ip_addr);
            return pos;
        }
        auto  enable()->int  override
        {
            
            if(If.pController->enableActuator(motor_id+1))
            {
                ZRCS_CONTROLLER_PRINTF("Enable actuator %d successfully!\n", motor_id + 1);
                return 1; 
            }
            return 0;
        }
        auto  disable()->int override
        {
            if(If.pController->disableActuator(motor_id+1))
            {
                ZRCS_CONTROLLER_PRINTF("Disable actuator %d successfully!\n", motor_id + 1);
                return 1; 
            }
            return 0;
        }
        auto  mode(std::uint8_t md)->int override
        {
               
               Actuator::ActuatorMode mode;
               switch (md) 
               { 
                    case 1:
                        mode = Actuator::ActuatorMode::Mode_None;
                        break;
                    case 2:
                        mode = Actuator::ActuatorMode::Mode_Cur;
                        break;
                    case 3:
                        mode = Actuator::ActuatorMode::Mode_Vel;
                        break;
                    case 4:
                        mode = Actuator::ActuatorMode::Mode_Pos;
                        break;
                    case 5:
                        mode = Actuator::ActuatorMode::Mode_Teaching;
                        break;
                    case 6:
                        mode = Actuator::ActuatorMode::Mode_Profile_Pos;
                        break;
                    case 7:
                        mode = Actuator::ActuatorMode::Mode_Profile_Vel;
                        break;
                    case 8:
                        mode = Actuator::ActuatorMode::Mode_Homing;
                        break;
                    default:
                        return 0;
                        break;
               }
               If.pController->activateActuatorMode(motor_id+1,mode,ip_addr);
               return 1;
        }
        auto clearError()->int override
        {
            
            If.pController->clearError(motor_id+1,ip_addr);
            return 1;
        }
      
    };
  class InnfosTransceive:Transceive
  {
      public:
      InnfosTransceive()
      {
      }
      ~InnfosTransceive()
      {
      }
      auto send(void)->void override
      {
      }
      auto receive(void)->void override
      {
      }
  };

  } 
  
  

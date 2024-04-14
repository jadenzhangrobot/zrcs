/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: controllr的一些抽象接口包括电机，传感器，io等
 * 
 */
#ifndef CONTROLLER_INTERFACE_H
#define CONTROLLER_INTERFACE_H
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <functional>
//#include "rtos/preempt_rt.h"
//#include "motor/rawsocketbus.h"
//#include "motor/urgazebo.h"
//#include "motor/cybergazebo.h"
//#include "motor/ureffort.h"
namespace controller
{
    class Motor
    {
        public:
            
            auto virtual controlWord()->std::uint16_t = 0;
            // auto virtual modeOfOperation()const->std::uint8_t = 0;
            // auto virtual targetPos(double j_val)->void;
            // auto virtual targetVel()const->double = 0;
            // auto virtual targetToq()const->double = 0;
            // auto virtual offsetVel()const->double = 0;
            // auto virtual offsetCur()const->double = 0;
            
            auto virtual setControlWord(std::uint16_t control_word)->void = 0;
            //auto virtual setModeOfOperation(std::uint8_t mode)->void = 0;
              
            // auto virtual setTargetVel(double vel)->void = 0;
             auto virtual setTargetToq(double toq)  ->int
              {


                return 0;
              };
            // auto virtual setOffsetVel(double vel)->void = 0;
            // auto virtual setOffsetToq(double toq)->void = 0;
            // auto virtual setErrorCode(std::int32_t code)->void = 0;

            auto virtual statusWord() ->std::uint16_t = 0;
            //auto virtual modeOfDisplay()const->std::uint8_t = 0;
              auto virtual errorCode()->uint32_t 
              {
                return 0;
              }
              auto virtual setTargetPos(double pos)->int
              {
                return 0;
              };
              virtual double actualPos()
              {


                return 0;
              };
      
             virtual double actualVel()
            {

              return 0;
            }
            

              auto virtual actualToq()->double
              {



                return 0; 
               }
            // auto virtual actualCur()const->double = 0;
            // auto virtual actualAddlPos()const->double = 0;
            // auto virtual velDiff()const->double = 0;
            // auto virtual setVelDiff(double vel)->void = 0;

          auto virtual clearError()->int
          {
                return 0;
          }   
          auto virtual disable()->int
          {
                return 0;
          }
          auto virtual enable()->int
          {
                return 0;
          }
          auto virtual home()->int 
          {
                return 0;
          }
          auto virtual mode(std::uint8_t md)->int
          {
                return 0;
          }
          auto virtual init()->int
          {
                return 0;
          }

          virtual ~Motor(){};
                    
    };
    class Io
    {  


       virtual ~Io(){};
    };
    class Sensor
    {


      virtual ~Sensor(){};
    };

    class Transceive
    {
        public:
          auto virtual send(void)->void =0;
          auto virtual receive(void)->void =0;
          auto virtual init()->int =0;
          virtual ~Transceive(){};
    };
    class Rtos
    {
    public:
       virtual ~Rtos(){};
        virtual void real_task(std::function<void()> strategy)=0;
        virtual void rtos_task_create(void)=0;
        
        virtual void rtos_task_join(void)=0;
      
        virtual void rtos_set_perioic(int perioic)=0;
        
        virtual std::uint64_t rtos_timer_read(void)
        {
              uint64_t time;
               return  time;
        }     
    };
    class Communication
    {
        virtual   Motor*  create_motor(void) =0;
        virtual   Io*     create_io(void)=0;
        virtual   Sensor* create_sensor(void)=0;
    };
    
    class Controller
    {  
        public:
        std::shared_ptr<Transceive> transceiver;
        std::shared_ptr<Rtos> rtos_;
        std::vector<std::unique_ptr<Motor>> motors;
       
    };
    // typedef std::unique_ptr<Controller> control;
}
#endif
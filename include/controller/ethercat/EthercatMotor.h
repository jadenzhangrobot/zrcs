
#ifndef ETHERCATMOTOR_H
#define ETHERCATMOTOR_H
#include "EthercatMaster.h"
#include "controller/ControllerInterface.h"
#include "controller/ParameterRead.h"
#include <cstdint>
#include <string>
namespace HWAL {
	#define pi 3.1415926
class EthercatMotor:Motor
{
     private:
		int ModeOffset;
		int TargetposOffset;
		int ControlOffset;
		int ActualPos;
		int StatusWord;
     public:
	    EthercatMaster* ethercatMaster;
  
        EthercatMotor(int id,EthercatMaster* ethercatMaster_,MotorConfig* motorConfig_):Motor(id,motorConfig_),ethercatMaster( ethercatMaster_)
        {
		        ModeOffset=findNumberOutputKey("controlMode");
				ControlOffset=findNumberOutputKey("ControlWord");
				TargetposOffset=findNumberOutputKey("TargetPosition");
				ActualPos=findNumberInputKey("ActualPosition");
				StatusWord=findNumberInputKey("StatusWord");
			
        }
		
            int findNumberOutputKey(std::string key)
			{
				auto it = ethercatMaster->OutputPdoInfoAndOffset[motorId].find(key);
				if (it !=  ethercatMaster->OutputPdoInfoAndOffset[motorId].end()) {
					return it->second;
				} else {
					throw std::runtime_error("Failed to find "+std::to_string(motorId)+key);
				}
            }
			 int findNumberInputKey(std::string key)
			{
				auto it = ethercatMaster->InputPdoInfoAndOffset[motorId].find(key);
				if (it != ethercatMaster->InputPdoInfoAndOffset[motorId].end()) 
				{
					return it->second;
				} else {
					throw std::runtime_error("Failed to find "+std::to_string(motorId)+key);
				}
            }
		  void setModeOfOperation(std::uint8_t md) override
          {             	
			   EC_WRITE_S8(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[motorId][ModeOffset],md);
          }
        double setTargetPos (double position) override
        {	  
			
			  target_pos_=position;
              EC_WRITE_S32(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[motorId][TargetposOffset],static_cast<int32_t>((target_pos_+pos_offset)*pos_factor/(2*pi)));
              return 0;  
        }
        double actualPos(void) override
        {   			 
             int32_t pos= EC_READ_S32(ethercatMaster->DomainRead+ethercatMaster->InputOffset[motorId][ActualPos]);

			 double pos_=static_cast<double>(pos);			
			return (pos_-pos_offset)/pos_factor*2*pi;
        }
        double actualVel(void) override
        {
		   
           return 0;
        }
        int  setTargetToq(double toq) override
        {  
			return 0;
        }
        double actualToq(void) override
        {
              return 0;
        }
       
        // std::uint16_t controlWord() override
        //  {
                
        //       //return EC_READ_U16(em.DomainWrite+em.OutputOffset[0]);
        //  }

         void setControlWord(std::uint16_t control_word) override
         {
                EC_WRITE_U16(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[motorId][ControlOffset], control_word ); 

         }

          std::uint16_t statusWord() override
         {
              
              return EC_READ_U16(ethercatMaster->DomainRead+ethercatMaster->InputOffset[motorId][StatusWord]);

         }

          int  disable() override
          {
			// control word
			// 0x06    0b xxxx xxxx 0xxx 0110    A: transition 2,6,8         Shutdown
			// 0x07    0b xxxx xxxx 0xxx 0111    B: transition 3             Switch ON
			// 0x0F    0b xxxx xxxx 0xxx 1111    C: transition 3             Switch ON
			// 0x00    0b xxxx xxxx 0xxx 0000    D: transition 7,9,10,12     Disable Voltage
			// 0x02    0b xxxx xxxx 0xxx 0000    E: transition 7,10,11       Quick Stop
			// 0x07    0b xxxx xxxx 0xxx 0111    F: transition 5             Disable Operation
			// 0x0F    0b xxxx xxxx 0xxx 1111    G: transition 4,16          Enable Operation
			// 0x80    0b xxxx xxxx 1xxx xxxx    H: transition 15            Fault Reset
			//
			// status word
			// 0x00    0b xxxx xxxx x0xx 0000    A: not ready to switch on     
			// 0x40    0b xxxx xxxx x1xx 0000    B: switch on disabled         
			// 0x21    0b xxxx xxxx x01x 0001    C: ready to switch on         
			// 0x23    0b xxxx xxxx x01x 0011    D: switch on                  
			// 0x27    0b xxxx xxxx x01x 0111    E: operation enabled          
			// 0x07    0b xxxx xxxx x00x 0111    F: quick stop active
			// 0x0F    0b xxxx xxxx x0xx 1111    G: fault reaction active
			// 0x08    0b xxxx xxxx x0xx 1000    H: fault
			//
			// 0x6F    0b 0000 0000 0110 1111
			// 0x4F    0b 0000 0000 0100 1111
			// disable change state to A/B/C/E to D

		auto status_word = statusWord();
         
		// check status A, now transition 1 automatically
		if ((status_word & 0x4F) == 0x00) {
			// this just set the initial control word...
		
			setControlWord(std::uint16_t(0x00));
			return 1;
		}
		// check status B, now keep and return
		else if ((status_word & 0x4F) == 0x40) {
			// transition 2 //
			
			setControlWord(std::uint16_t(0x00));
			return 0;
		}
		// check status C, now transition 7
		else if ((status_word & 0x6F) == 0x21) {
			// transition 3 //
		
			setControlWord(std::uint16_t(0x00));
			return 0;
		}
		// check status D, now transition 10
		else if ((status_word & 0x6F) == 0x23) {
			
			setControlWord(std::uint16_t(0x06));//change to 0x06 for cooldrive
			return 3;
		}
		// check status E, now transition 9
		else if ((status_word & 0x6F) == 0x27) {
			// transition 5 //
			
			setControlWord(std::uint16_t(0x07));//change to 0x07 for cooldrive
			return 4;
		}
		// check status F, now transition 12
		else if ((status_word & 0x6F) == 0x07) {
			
			setControlWord(std::uint16_t(0x00));
			return 5;
		}
		// check status G, now transition 14
		else if ((status_word & 0x4F) == 0x0F) {
		
			setControlWord(std::uint16_t(0x00));
			return 6;
		}
		// check status H, now transition 15
		else if ((status_word & 0x4F) == 0x08) {
			// transition 4 //
			setControlWord(std::uint16_t(0x80));
			return 7;
		}
		// unknown status
		else {
			return -1;
		}
          }
        int enable() override
          {
		// control word
		// 0x06    0b xxxx xxxx 0xxx 0110    A: transition 2,6,8       Shutdown
		// 0x07    0b xxxx xxxx 0xxx 0111    B: transition 3           Switch ON
		// 0x0F    0b xxxx xxxx 0xxx 1111    C: transition 3           Switch ON
		// 0x00    0b xxxx xxxx 0xxx 0000    D: transition 7,9,10,12   Disable Voltage
		// 0x02    0b xxxx xxxx 0xxx 0000    E: transition 7,10,11     Quick Stop
		// 0x07    0b xxxx xxxx 0xxx 0111    F: transition 5           Disable Operation
		// 0x0F    0b xxxx xxxx 0xxx 1111    G: transition 4,16        Enable Operation
		// 0x80    0b xxxx xxxx 1xxx xxxx    H: transition 15          Fault Reset
		// 
		// status word
		// 0x00    0b xxxx xxxx x0xx 0000    A: not ready to switch on     
		// 0x40    0b xxxx xxxx x1xx 0000    B: switch on disabled         
		// 0x21    0b xxxx xxxx x01x 0001    C: ready to switch on         
		// 0x23    0b xxxx xxxx x01x 0011    D: switch on                  
		// 0x27    0b xxxx xxxx x01x 0111    E: operation enabled          
		// 0x07    0b xxxx xxxx x00x 0111    F: quick stop active
		// 0x0F    0b xxxx xxxx x0xx 1111    G: fault reaction active
		// 0x08    0b xxxx xxxx x0xx 1000    H: fault
		// 
		// 0x6F    0b 0000 0000 0110 1111
		// 0x4F    0b 0000 0000 0100 1111
		// enable change state to A/B/C/D/F/G/H to E
       
		auto status_word = statusWord();
	
		// check status A
		if ((status_word & 0x4F) == 0x00) {
			return 1;
		}
		// check status B, now transition 2
		else if ((status_word & 0x4F) == 0x40) 
		{
			// transition 2 //			
			setControlWord(std::uint16_t(0x06));
			return 2;
		}
		// check status C, now transition 3
		else if ((status_word & 0x6F) == 0x21)
		 {
			// transition 3 //
			
			setControlWord(std::uint16_t(0x07));
			return 3;
		}
		// check status D, now transition 4
		else if ((status_word & 0x6F) == 0x23) {
			// transition 4 //		   
					setControlWord(std::uint16_t(0x0F));
					// check mode to set correct pos, vel or cur //
					switch (0x08) 
					{
					case 0x08: setTargetPos(actualPos()); break;
				
					default: setTargetPos(actualPos());
					}
			return 4;
		}
		// check status E, now keep status
		else if ((status_word & 0x6F) == 0x27)
		{
			// check if need wait //
			// if (--imp_->waiting_count_left > 0) return 5;
			// // now return normal
			// else return 5;
            return 5;
		}
		// check status F, now transition 12
		else if ((status_word & 0x6F) == 0x07) 
		{
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x00));
			setControlWord(std::uint16_t(0x00));
			return 6;
		}
		// check status G, now transition 14
		else if ((status_word & 0x4F) == 0x0F)
		{
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x00));
			setControlWord(std::uint16_t(0x00));
			return 7;
		}
		// check status H, now transition 15
		else if ((status_word & 0x4F) == 0x08) 
		{
			// transition 4 //
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x80));
			setControlWord(std::uint16_t(0x80));
			return 8;
		}
		// unknown status
		else
		{
			return -1;
		}
          }
    };
}


#endif

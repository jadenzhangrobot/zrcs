
#ifndef ETHERCATMOTOR_H
#define ETHERCATMOTOR_H
#ifdef REALTIME
	#include "EthercatMaster.h"
#endif
#include "controller/ControllerInterface.h"

#include <cstdint>
#include <string>
namespace ZrcsHardware {
	
class EthercatMotor:Axis
{
     private:
		int ModeOffset;
		int TargetposOffset;
		int ControlOffset;
		int ActualPos;
		int StatusWord;
		int motorId;
     public:
	    EthercatMaster* ethercatMaster;
  
        EthercatMotor(int id,EthercatMaster* ethercatMaster_):motorId(id),ethercatMaster( ethercatMaster_)
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
			if (it != ethercatMaster->InputPdoInfoAndOffset[motorId].end()){
				return it->second;
			} else {
				throw std::runtime_error("Failed to find "+std::to_string(motorId)+key);
			}
		}
		void setModeOfOperation(std::uint8_t md) override
		{             	
			EC_WRITE_S8(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[motorId][ModeOffset],md);
		}
        void setEncoderTargetPos (double position) override
        {	  
              EC_WRITE_S32(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[motorId][TargetposOffset],static_cast<int32_t>(position));
        }
        double encoderActualPos(void) override
        {   			 			 
             int32_t pos= EC_READ_S32(ethercatMaster->DomainRead+ethercatMaster->InputOffset[motorId][ActualPos]);
			 return static_cast<double>(pos);						
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
         
	
		if ((status_word & 0x4F) == 0x00) {
			
		
			setControlWord(std::uint16_t(0x00));
			return 1;
		}
		
		else if ((status_word & 0x4F) == 0x40) {
			// transition 2 //
			
			setControlWord(std::uint16_t(0x00));
			return 0;
		}
		
		else if ((status_word & 0x6F) == 0x21) {
			// transition 3 //
		
			setControlWord(std::uint16_t(0x00));
			return 0;
		}
		
		else if ((status_word & 0x6F) == 0x23) {
			
			setControlWord(std::uint16_t(0x06));//change to 0x06 for cooldrive
			return 3;
		}
		
		else if ((status_word & 0x6F) == 0x27) {
			
			
			setControlWord(std::uint16_t(0x07));//change to 0x07 for cooldrive
			return 4;
		}
		
		else if ((status_word & 0x6F) == 0x07) {
			
			setControlWord(std::uint16_t(0x00));
			return 5;
		}
		
		else if ((status_word & 0x4F) == 0x0F) {
		
			setControlWord(std::uint16_t(0x00));
			return 6;
		}
		
		else if ((status_word & 0x4F) == 0x08) {
			
			setControlWord(std::uint16_t(0x80));
			return 7;
		}
		
		else {
			return -1;
		}
          }
         int  switchOn() override
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
	
	
		if ((status_word & 0x4F) == 0x00) {
			return 1;
		}
		
		else if ((status_word & 0x4F) == 0x40) 
		{
				
			setControlWord(std::uint16_t(0x06));
			return 2;
		}
		
		else if ((status_word & 0x6F) == 0x21)
		 {
		
			setControlWord(std::uint16_t(0x07));
			return 3;
		}
		
		else if ((status_word & 0x6F) == 0x07) 
		{
			
			setControlWord(std::uint16_t(0x00));
			return 6;
		}
		
		else if ((status_word & 0x4F) == 0x0F)
		{
			
			setControlWord(std::uint16_t(0x00));
			return 7;
		}
		
		else if ((status_word & 0x4F) == 0x08) 
		{
			
			setControlWord(std::uint16_t(0x80));
			return 8;
		}
		else
		{
			return -1;
		}
          }
    };
}


#endif

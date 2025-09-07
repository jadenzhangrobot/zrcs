
#ifndef ETHERCATMOTOR_H
#define ETHERCATMOTOR_H
#include "EthercatMaster.h"
#include "controller/ControllerInterface.h"
#include <cstdint>
#include <string>
#include <sys/types.h>
namespace ZrcsHardware {
	
class EthercatMotor:public Servo
{
     private:
		int ModeOffset;
		int TargetposOffset;
		int ControlOffset;
		int ActualPos;
		int StatusWord;

	 bool power=false;
	 bool powerStatus=false;
     public:
	    EthercatMaster* ethercatMaster;
        int slaveId;
        EthercatMotor(int id):slaveId(id),ethercatMaster(new EthercatMaster())
        {			   
		        ModeOffset=findNumberOutputKey("controlMode");
				ControlOffset=findNumberOutputKey("ControlWord");
				TargetposOffset=findNumberOutputKey("TargetPosition");
				ActualPos=findNumberInputKey("ActualPosition");
				StatusWord=findNumberInputKey("StatusWord");			
        }
		~EthercatMotor()
		{
			delete ethercatMaster;
		}
		
		int findNumberOutputKey(std::string key)
		{
			auto it = ethercatMaster->OutputPdoInfoAndOffset[slaveId].find(key);
			if (it !=  ethercatMaster->OutputPdoInfoAndOffset[slaveId].end()) {
				return it->second;
			} else {
				throw std::runtime_error("Failed to find "+std::to_string(slaveId)+key);
			}
			
		}
		int findNumberInputKey(std::string key)
		{
			auto it = ethercatMaster->InputPdoInfoAndOffset[slaveId].find(key);
			if (it != ethercatMaster->InputPdoInfoAndOffset[slaveId].end()){
				return it->second;
			} else {
				throw std::runtime_error("Failed to find "+std::to_string(slaveId)+key);
			}
		}
		MC_SERVO_CODE setMode(Cia402Mode mode) override
		{             	
			EC_WRITE_S8(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[slaveId][ModeOffset],mode);
			return SERVONOERROR;
		}
        MC_SERVO_CODE setPos(int32_t position) override
        {	  
              EC_WRITE_S32(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[slaveId][TargetposOffset],static_cast<int32_t>(99));
			  return SERVONOERROR;
        }
        int32_t pos(void) override
        {   			 			 
             int32_t pos= EC_READ_S32(ethercatMaster->DomainRead+ethercatMaster->InputOffset[slaveId][ActualPos]);
			 return static_cast<double>(pos);						
        }
		
        int32_t vel(void) override
        {
		   
           return 0;
        }
        
		MC_SERVO_CODE resetError(bool& isDone) override
		{

		}

		//  void runCycle(void) override
		// {

		// }
		void emergStop(void) override
		{
			
		}

       
         std::uint16_t controlWord()
         {
                
              //return EC_READ_U16(em.DomainWrite+em.OutputOffset[0]);
         }

         void setControlWord(std::uint16_t control_word)
         {
                EC_WRITE_U16(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[slaveId][ControlOffset], control_word ); 
         }

          std::uint16_t statusWord()
         {
              
              return EC_READ_U16(ethercatMaster->DomainRead+ethercatMaster->InputOffset[slaveId][StatusWord]);

         }

		MC_SERVO_CODE setPower(bool powerSwitch) override
		{
			power=powerSwitch;
            return SERVONOERROR;
		}
		bool getPower()
		{
			return powerStatus;
		}
        void  runCycle()override
		{
            if (power) 
			{
			    int result=  motorPowerOn();
				if (result==3) 
				{
				      powerStatus=true;
				}
			}
		    else 
			{
			    int result= motorPowerOff();
				if (result==0) 
				{
				    powerStatus=false;
				}
			}
             
		}

        int  motorPowerOff(void)
        {

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
		
        }
          int  motorPowerOn(void) 
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
       }
};
}


#endif

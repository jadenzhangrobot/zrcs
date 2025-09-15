
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
        int32_t position_;      // 当前位置
        int32_t lastPosition_;      // 上个周期位置
        int32_t velocity_;      // 当前速度
        int32_t lastVelocity_;      // 上个周期速度
        int32_t acceleration_;  // 当前加速度
     public:
	    EthercatMaster* ethercatMaster;
        int slaveId;
        EthercatMotor(int id, EthercatMaster* master):slaveId(id),ethercatMaster(master)
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
              EC_WRITE_S32(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[slaveId][TargetposOffset],position);
			  return SERVONOERROR;
        }
        int32_t pos(void) override
        {   	
			 lastPosition_=position_;		 			 
             position_= EC_READ_S32(ethercatMaster->DomainRead+ethercatMaster->InputOffset[slaveId][ActualPos]);
			 
			 return position_;		
        }
		
        int32_t vel(void) override
        {
		   
           lastVelocity_=velocity_;
            velocity_=(position_-lastPosition_)*1000/cycletime;           
            return velocity_;
        }
		 int32_t acc(void) override 
        {
            // CoppeliaSim 中通常不直接提供加速度读取
             acceleration_=(velocity_-lastVelocity_)*1000/cycletime;
            return acceleration_;
        }
        
		bool resetError(void) override
		{
			auto status_word = statusWord();
            if ((status_word & 0x4F) == 0x08) 
			{
			  setControlWord(std::uint16_t(0x80));
			  return true;
		    }
		    return false;   
		}
		void emergStop(void) override
		{
			
		}
         void setControlWord(std::uint16_t control_word)
         {
                EC_WRITE_U16(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[slaveId][ControlOffset], control_word ); 
         }

          std::uint16_t statusWord()
         {
              
              return EC_READ_U16(ethercatMaster->DomainRead+ethercatMaster->InputOffset[slaveId][StatusWord]);

         }
		bool enable(void) override
		{
			    auto status_word = statusWord();
                if ((status_word & 0x6F) == 0x23) 
				{								
					setControlWord(std::uint16_t(0x0F));							
					return true;
				}

				return false;
		}
		bool disable(void) override
		{
			auto status_word = statusWord();
			if ((status_word & 0x6F) == 0x27) 
			{								
				setControlWord(std::uint16_t(0x07));						
				return true;
			}

			return false;
		}

		bool getState()
		{
			  auto status_word = statusWord();
			  if ((status_word & 0x6F) == 0x27)
			  {
				  return true;
			  }
			  return false;
		}
        void  runCycle() override
		{           
			auto status_word = statusWord();


			 if ((status_word & 0x4F) == 0x40) 
			{
						
				setControlWord(std::uint16_t(0x06));
			
			}
			
			else if ((status_word & 0x6F) == 0x21) 
			{
				
				
				setControlWord(std::uint16_t(0x07));
				
			}
			// else if ((status_word & 0x4F) == 0x08) 
			// {
			//   setControlWord(std::uint16_t(0x80));
		    // }        
		}          
};
}


#endif

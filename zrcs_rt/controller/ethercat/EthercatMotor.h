
#pragma once

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

        // CiA402 目标状态: 只有 enable()/disable() 能改; runCycle() 据此推进状态机,
        // 无请求(Hold)时保持现状, 禁止后台自行使能。
        enum class EnableCmd : uint8_t { Hold = 0, Enable, Disable };
        EnableCmd enableCmd_{EnableCmd::Hold};
        // 故障复位边沿锁存: 每个故障事件只发一次 Fault Reset(0x80), 不每周期重发。
        bool faultResetSent_{false};

        // CiA402 状态字(按掩码解析)。用枚举表达, 主逻辑 switch 更清爽。
        enum class DriveState : uint16_t {
            Fault             = 0x08,   // 驱动器故障
            SwitchOnDisabled  = 0x40,   // 只上了控制电
            ReadyToSwitchOn   = 0x21,   // 允许加主电
            SwitchedOn        = 0x27,   // 已通电, 未使能
            OperationEnabled  = 0x23,   // 可运行(真正的"已使能")
            Unknown           = 0xFFFF, // 过渡态/未识别
        };

       
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
		~EthercatMotor() = default;
		
		int findNumberOutputKey(const std::string& key)
		{
			auto it = ethercatMaster->OutputPdoInfoAndOffset[slaveId].find(key);
			if (it !=  ethercatMaster->OutputPdoInfoAndOffset[slaveId].end()) {
				return it->second;
			} else {
				throw std::runtime_error("Failed to find "+std::to_string(slaveId)+key);
			}
			
		}
		int findNumberInputKey(const std::string& key)
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
        MC_SERVO_CODE setVel(int32_t vel) override 
        {
            // TODO: Implement velocity mode for EtherCAT
            return SERVONOERROR;
        }

        MC_SERVO_CODE setTorque(int32_t torque) override 
        {
            // TODO: Implement torque mode for EtherCAT
            return SERVONOERROR;
        }

        MC_SERVO_CODE setPos(int32_t pos) override
        {	  lastPosition_=position_;
              position_ = pos;
              EC_WRITE_S32(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[slaveId][TargetposOffset],pos);
			  return SERVONOERROR;
        }
        int32_t pos(void) override
        {   	
			 	 			 
            return   EC_READ_S32(ethercatMaster->DomainRead+ethercatMaster->InputOffset[slaveId][ActualPos]);
			 
			 		
        }
		
        int32_t vel(void) override
        {
		   
            lastVelocity_=velocity_;
            velocity_=(position_-lastPosition_)*1000/cycletime;           
            return velocity_;
        }
		 int32_t acc(void) override 
        {
            // 驱动通常不直接提供加速度读取
             acceleration_=(velocity_-lastVelocity_)*1000/cycletime;
            return acceleration_;
        }
		int32_t torque(void) override 
        {
            return 0; // TODO: Implement torque reading
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
			enableCmd_ = EnableCmd::Enable;
			return (statusWord() & 0x6F) == 0x23;   // 是否已处于 Operation Enabled
		}
		bool disable(void) override
		{
			enableCmd_ = EnableCmd::Disable;
			return (statusWord() & 0x6F) != 0x23;   // 是否已退出 Operation Enabled
		}
        

        DriveState getState(std::uint16_t status_word)
        {
            if ((status_word & 0x4F) == 0x08) return DriveState::Fault;
            if ((status_word & 0x4F) == 0x40) return DriveState::SwitchOnDisabled;
            if ((status_word & 0x6F) == 0x21) return DriveState::ReadyToSwitchOn;
            if ((status_word & 0x6F) == 0x27) return DriveState::SwitchedOn;
            if ((status_word & 0x6F) == 0x23) return DriveState::OperationEnabled;
            return DriveState::Unknown;
        }
		/// 驱动器是否已真正可运行(Operation Enabled)。
		bool isEnabled()
		{
			return getState(statusWord()) == DriveState::OperationEnabled;
		}
		bool isDisabled()
		{
			return getState(statusWord()) == DriveState::SwitchedOn;
		}
        void  runCycle() override
		{
			// 每拍只读一次状态字: 两次 EC 读之间状态可能已变化,
			// 而且省掉一次无谓的 PDO 读。
			const DriveState state = getState(statusWord());

			if (state == DriveState::SwitchOnDisabled)
			{
				// 0x06 Shutdown: 0x40 -> 0x21
				setControlWord(std::uint16_t(0x06));
			}
			else if (state == DriveState::ReadyToSwitchOn)
			{
				setControlWord(std::uint16_t(0x07));
			}
			// 注意: SwitchedOn(0x27) 还需下发 EnableOperation(0x0F) 才能进入
			// OperationEnabled(0x23); 缺这一步 enable() 永远不会返回 true。
		}

};
}


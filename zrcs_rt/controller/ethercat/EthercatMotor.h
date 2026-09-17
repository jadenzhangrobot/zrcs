
#pragma once

#include "EthercatMaster.h"
#include "controller/Servo.h"
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


        // CiA402 状态字(按掩码解析)。用枚举表达, 主逻辑 switch 更清爽。
        // 枚举值 = 状态字按掩码屏蔽后的比较值: 故障族用 0x4F(bit6 禁止上电 + bit3 故障),
        // 其余状态用 0x6F(再多屏蔽 bit5 快停)。bit4 电压使能 / bit7 警告 / bit9 远程 /
        // bit10 到位与状态无关, 必须先屏蔽再比较, 否则状态永远解析不出来。
        // 注意 SwitchedOn(0x23) 与 OperationEnabled(0x27) 只差 bit2: 前者电机不出力。
        
      enum class DriveState : uint16_t 
		{
            NotReadyToSwitchOn = 0x00,   // 上电自检/加载参数中, 不接受任何命令
            QuickStopActive    = 0x07,   // 快停中(bit5=0)
            Fault              = 0x08,   // 驱动器故障, 只认 Fault Reset
            FaultReaction      = 0x0F,   // 故障反应中, 会自动过渡到 Fault
            ReadyToSwitchOn    = 0x21,   // 允许加主电
            SwitchedOn         = 0x23,   // 已通电, 未使能 —— 电机仍不出力
            OperationEnabled   = 0x27,   // 可运行(真正的"已使能")
            SwitchOnDisabled   = 0x40,   // 只上了控制电
            Unknown            = 0xFFFF, // 过渡态/未识别
        };
     public:
	    EthercatMaster* ethercatMaster;
        int slaveId;
		DriveState getState(std::uint16_t status_word)
        {
            // 故障族先判(掩码 0x4F): 故障位/禁止上电位与其余状态互斥。
            if ((status_word & 0x4F) == 0x00) return DriveState::NotReadyToSwitchOn;
            if ((status_word & 0x4F) == 0x08) return DriveState::Fault;
            if ((status_word & 0x4F) == 0x0F) return DriveState::FaultReaction;
            if ((status_word & 0x4F) == 0x40) return DriveState::SwitchOnDisabled;
            // 其余状态用掩码 0x6F: 0x23 只是 SwitchedOn(电机不出力), 0x27 才是可运行。
            if ((status_word & 0x6F) == 0x07) return DriveState::QuickStopActive;
            if ((status_word & 0x6F) == 0x21) return DriveState::ReadyToSwitchOn;
            if ((status_word & 0x6F) == 0x23) return DriveState::SwitchedOn;
            if ((status_word & 0x6F) == 0x27) return DriveState::OperationEnabled;
            return DriveState::Unknown;
        }
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
        
		/// @brief 请求清除驱动器故障(Fault Reset)。
		///        这里只置位请求, 0x80 由 runCycle() 在 Fault 态下发一拍的上升沿。
		/// @return 无故障可复位时同样返回 true。返回 false 会让 Axis::resetError()
		///         提前 return, 连轴的软件错误码都清不掉, 调度层会一直卡在 ERROR_STATE。
		bool resetError(void) override
		{
			if (getState(statusWord()) == DriveState::Fault)
			{
				setControlWord(0x80);
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
			if (getState(statusWord()) == DriveState::SwitchedOn)
			{
				setControlWord(0x0F); // Shutdown
				return true;
			}
			return false;
		}

		/// @brief 请求失能: runCycle() 下一拍开始退出 OperationEnabled
		///        (OperationEnabled 先 Shutdown 按斜坡停机, 再退到 ReadyToSwitchOn)。
		/// @return 是否受理。驱动器本来就处于故障/未上电时失能无意义, 同样算受理。
		bool disable(void) override
		{
			if (getState(statusWord()) == DriveState::OperationEnabled)
			{
				setControlWord(0x07); // Shutdown	
				return true;
			}
			return false;
		}
        

      
		/// 驱动器实时状态是否已真正可运行(Operation Enabled)。
		/// 注意与 enable() 的区别: 那是"请求是否受理", 这是驱动器上报的瞬时状态,
		/// 请求之后要过 2~3 拍才会变成 true。
		bool isEnabled()
		{
			if (getState(statusWord()) == DriveState::OperationEnabled)
			{
				return true;
			}
			return false;
		}
		bool isDisabled()
		{
			if (getState(statusWord()) == DriveState::SwitchedOn)
			{
				return true;
			}
			return false;
		}
        Servo::ServoState runCycle() override
		{
			// 每拍只读一次状态字: 两次 EC 读之间状态可能已变化,
			// 而且省掉一次无谓的 PDO 读。
			const DriveState state = getState(statusWord());
			switch (state)
			{
			case DriveState::NotReadyToSwitchOn:
				// 上电自检/加载参数中, 不接受任何命令, 等驱动器自己走到 SwitchOnDisabled。
				return Servo::ServoState::NotReady;
			case DriveState::SwitchOnDisabled:
				// 0x40 --Shutdown(0x06)--> 0x21。writeControlCmd
				// 只有收到使能请求才往上走, 否则维持在"只上了控制电", 禁止后台自行使能。
				setControlWord(0x06);
				return Servo::ServoState::Disabled;
			case DriveState::ReadyToSwitchOn:
				setControlWord(0x07);
				return Servo::ServoState::Disabled;
			case DriveState::QuickStopActive:
				// 0x07: 快停中(由 Quick Stop(0x02) 或驱动器内部事件触发)。
				// 0x0F 退出快停回到可运行; 否则按 0x00 断主电落到 SwitchOnDisabled。
				return Servo::ServoState::Stopping;
			case DriveState::Fault:
				return Servo::ServoState::Fault;
			case DriveState::OperationEnabled:
				return Servo::ServoState::Enabled;
			case DriveState::SwitchedOn:
				return Servo::ServoState::Disabled;
			case DriveState::Unknown:
			default:
				// 过渡态或状态字位组合非法: 什么都不发, domain 里仍是上一拍的控制字,
				// 避免用错误的命令把驱动器推到别处; 下一拍状态通常就明确了。
				return Servo::ServoState::Unknown;
			}
		}

};
}


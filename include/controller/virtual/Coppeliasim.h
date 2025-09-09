#ifndef COPPELIASIM_H
#define COPPELIASIM_H
#ifndef REALTIME
#include "controller/ControllerInterface.h"
#include <cstdint>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <memory>

extern "C" {
    #include "extApi.h"
    #include "simLib/simConst.h"
}

namespace ZrcsHardware 
{
    class CoppeliasimMaster
    {
            // 连接状态
    public:
        int clientID_;         // CoppeliaSim 客户端ID
        std::vector<int> jointHandles_;  // 关节句柄
        std::vector<std::string> jointNames_;  // 关节名称
         bool connected_ ;  
        ~CoppeliasimMaster()
        {
            if (connected_) {
                simxFinish(clientID_);
            }
        }
         CoppeliasimMaster():connected_(false)
        {
            
        }

        void init()
        {
             jointNames_ = {
                "UR5_joint1", "UR5_joint2", "UR5_joint3",
                "UR5_joint4", "UR5_joint5", "UR5_joint6"
            };
            
            // 连接到 CoppeliaSim
            clientID_ = simxStart((simxChar*)"127.0.0.1", 8888, true, true, 2000, 5);
            if (clientID_ != -1)
            {
                connected_ = true;
                std::cout << "Connected to CoppeliaSim remote API server" << std::endl;

                // 获取场景中的对象数量
                int objectCount;
                int* objectHandles;
                int ret = simxGetObjects(clientID_, sim_handle_all, &objectCount, &objectHandles, simx_opmode_blocking);
                if (ret == simx_return_ok)
                    std::cout << "Number of objects in the scene: " << objectCount << std::endl;
                else
                    throw std::runtime_error("远程API调用失败,错误码: " + std::to_string(ret));

                // 获取关节句柄
                jointHandles_.resize(jointNames_.size());
                for (size_t i = 0; i < jointNames_.size(); ++i) 
                {
                    int returnCode = simxGetObjectHandle(clientID_, jointNames_[i].c_str(), &jointHandles_[i], simx_opmode_blocking);
                    if (returnCode != simx_return_ok) {
                        throw std::runtime_error("错误：无法获取关节" + std::to_string(returnCode));
                        connected_ = false;
                    } else {
                        std::cout << "成功获取句柄：" << jointNames_[i] << " -> " << jointHandles_[i] << std::endl;
                    }
                }
                
                // 初始化数据流
                if (connected_) {
                    for (int handle : jointHandles_) 
                    {
                        float angle;
                        simxGetJointPosition(clientID_, handle, &angle, simx_opmode_streaming);
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 短暂等待
                }
            }
            else
            {
                throw std::runtime_error("无法连接到 CoppeliaSim 服务器");
                connected_ = false;
            }

        }
    }; 
     

    class Coppeliasim: public Servo
    {
    private:
        int32_t position_;      // 当前位置
        int32_t lastPosition_;      // 上个周期位置
        int32_t velocity_;      // 当前速度
        int32_t lastVelocity_;      // 上个周期速度
        int32_t acceleration_;  // 当前加速度
    
       static inline CoppeliasimMaster master_;
        // CoppeliaSim 连接相关
       int slaveId_;
       
    public:
        Coppeliasim(int slaveId) : position_(0), velocity_(0), acceleration_(0),slaveId_(slaveId)
        {
            if (!master_.connected_) 
            {
                master_.init();
            }
        }
        
        virtual ~Coppeliasim()
        {
            
        }
         // 必须实现的纯虚函数
        
        virtual MC_SERVO_CODE setPos(int32_t pos) override
        {
           
            if (!master_.connected_ || master_.clientID_ == -1) 
            {
                return MC_SERVO_CODE::SERVONOERROR;
            }      
               
            // 将位置转换为弧度（假设输入是编码器计数）
             float targetPosition =  pos / 1000000.0f;
         
            // 设置关节位置到 CoppeliaSim
            if (slaveId_ >= 0 && slaveId_ < static_cast<int>(master_.jointHandles_.size())) 
            {
                int ret = simxSetJointTargetPosition(master_.clientID_, master_.jointHandles_[slaveId_], targetPosition, simx_opmode_oneshot);
                
                if (ret == simx_return_ok||ret==1) 
                {
                    return MC_SERVO_CODE::SERVONOERROR;
                } else {
                   std::cerr << "设置关节位置失败，错误码: " << ret << std::endl;
                   return MC_SERVO_CODE::SERVONOERROR;
                }
            } 
            else 
            {
                std::cerr << "无效的从站ID: " << slaveId_ << std::endl;
                 return MC_SERVO_CODE::SERVONOERROR;
             }
             
        }
            
        virtual MC_SERVO_CODE setVel(int32_t vel) override 
        {
            velocity_ = vel;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setTorque(int32_t torque) override 
        {
           // torque_ = torque;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setMode(Cia402Mode mode) override
        {
            //mode_ = mode;
            return MC_SERVO_CODE::SERVONOERROR;
        }
       
        
        
        virtual bool readVal(int index, double& value) override 
        {
            // 虚拟实现：根据索引返回相应的值
            switch(index) 
            {
                case 0: value = position_; return true;
                case 1: value = velocity_; return true;
                case 2: value = acceleration_; return true;
                //case 3: value = torque_; return true;
                default: return false;
            }
        }
        
        virtual bool writeVal(int index, double value) override 
        {
            // 虚拟实现：根据索引设置相应的值
            switch(index)
            {
                case 0: position_ = value; return true;
                case 1: velocity_ = value; return true;
                case 2: acceleration_ = value; return true;
               // case 3: torque_ = value; return true;
                default: return false;
            }
        }
        
        
        virtual void runCycle() override 
        {
            // 虚拟实现：模拟运行周期
            // 在实际实现中，这里会执行伺服控制循环
        }
        
        virtual void emergStop(void) override
        {
            // 虚拟实现：紧急停止
            velocity_ = 0.0;
            acceleration_ = 0.0;
            //powerStatus_ = false;
        }
        
      
      

        virtual int32_t pos(void) override 
        {
            
            if (!master_.connected_ || master_.clientID_ == -1) 
            {
                return position_; // 返回缓存的位置
            }
             lastPosition_=position_;
            // 从 CoppeliaSim 读取实际关节位置
            if (slaveId_ >= 0 && slaveId_ < static_cast<int>(master_.jointHandles_.size()))
             {
                float currentPosition;
                int ret = simxGetJointPosition(master_.clientID_, master_.jointHandles_[slaveId_], &currentPosition, simx_opmode_blocking);
              
                if (ret == simx_return_ok||ret==1)
                {
                    // 将弧度转换为编码器计数
                    position_ = static_cast<int32_t>(currentPosition*1000000); // 根据实际编码器分辨率调整
                    return position_;
                } 
                else 
                {
                    // 如果读取失败，返回缓存的位置
                    return position_;
                }
            } 
            else 
            {
                return position_;
            }
         }
        
        virtual int32_t vel(void) override 
        {
            lastVelocity_=velocity_;
            velocity_=(position_-lastPosition_)*1000/cycletime;           
            return velocity_;
        
        }
        
        virtual int32_t acc(void) override 
        {
            // CoppeliaSim 中通常不直接提供加速度读取
             acceleration_=(velocity_-lastVelocity_)*1000/cycletime;
            return acceleration_;
        }
        
    };
}
#endif
#endif
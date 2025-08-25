#ifndef VIRTUALSERVO_H
#define VIRTUALSERVO_H
#include "controller/ControllerInterface.h"
#include<thread>
extern "C" {
    #include "extApi.h"
    #include "simLib/simConst.h"
}

namespace ZrcsHardware 
{
    class Coppeliasim: public Servo
    {
    private:
        double position_;      // 当前位置
        double velocity_;      // 当前速度
        double acceleration_;  // 当前加速度
        double torque_;        // 当前扭矩
        bool powerStatus_;     // 电源状态
        Cia402Mode mode_;      // 控制模式
        std::pmr::vector<int32_t> p;
    public:
        Coppeliasim(int slaveId) : position_(0.0), velocity_(0.0), acceleration_(0.0), torque_(0.0), powerStatus_(false), mode_(Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION)
            {
                int clientID=simxStart((simxChar*)"127.0.0.1",8888,true,true,2000,5);
                if (clientID!=-1)
                {
                        printf("Connected to remote API server\n");

                        // Now try to retrieve data in a blocking fashion (i.e. a service call):
                        int objectCount;
                        int* objectHandles;
                        int ret=simxGetObjects(clientID,sim_handle_all,&objectCount,&objectHandles,simx_opmode_blocking);
                        if (ret==simx_return_ok)
                            printf("Number of objects in the scene: %d\n",objectCount);
                        else
                            printf("Remote API function call returned with error code: %d\n",ret);

                    
                        std::vector<std::string> jointNames = {
                        "UR5_joint1", "UR5_joint2", "UR5_joint3",
                        "UR5_joint4", "UR5_joint5", "UR5_joint6"
                    };
                    std::vector<int> jointHandles(jointNames.size());
                    for (size_t i = 0; i < jointNames.size(); ++i) 
                    {
                        // simx_opmode_blocking 确保函数会等待服务器的响应
                        int returnCode = simxGetObjectHandle(clientID, jointNames[i].c_str(), &jointHandles[i], simx_opmode_blocking);
                        if (returnCode != simx_return_ok) {
                            std::cerr << "错误：无法获取关节 '" << jointNames[i] << "' 的句柄。请检查关节名称是否正确。\n";
                            //simxFinish(clientID); // 关闭连接
                            //return 1;
                        }
                        std::cout << "成功获取句柄：" << jointNames[i] << " -> " << jointHandles[i] << std::endl;
                    }
                    // 为了让服务器有时间准备数据流，先用 streaming 模式请求一次
                      for (int handle : jointHandles) 
                      {
                        float angle;
                        simxGetJointPosition(clientID, handle, &angle, 5);
                      }
                      std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 短暂等待

                }
            }
        
        virtual ~Coppeliasim()
        {
            
        }
        
        // 必须实现的纯虚函数
        virtual MC_SERVO_CODE setPower(bool powerStatus) override {
            powerStatus_ = powerStatus;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setPos(int32_t pos) override {
            position_ = pos;
            p.push_back(pos);
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setVel(int32_t vel) override {
            velocity_ = vel;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setTorque(int32_t torque) override {
            torque_ = torque;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual MC_SERVO_CODE setMode(Cia402Mode mode) override {
            mode_ = mode;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual int32_t pos(void) override {
            return position_;
        }
        
        virtual int32_t vel(void) override {
            return velocity_;
        }
        
        virtual int32_t acc(void) override {
            return acceleration_;
        }
        
        virtual int32_t torque(void) override {
            return torque_;
        }
        
        virtual bool readVal(int index, double& value) override {
            // 虚拟实现：根据索引返回相应的值
            switch(index) {
                case 0: value = position_; return true;
                case 1: value = velocity_; return true;
                case 2: value = acceleration_; return true;
                case 3: value = torque_; return true;
                default: return false;
            }
        }
        
        virtual bool writeVal(int index, double value) override {
            // 虚拟实现：根据索引设置相应的值
            switch(index) {
                case 0: position_ = value; return true;
                case 1: velocity_ = value; return true;
                case 2: acceleration_ = value; return true;
                case 3: torque_ = value; return true;
                default: return false;
            }
        }
        
        virtual MC_SERVO_CODE resetError(bool& isDone) override {
            // 虚拟实现：总是成功重置错误
            isDone = true;
            return MC_SERVO_CODE::SERVONOERROR;
        }
        
        virtual void runCycle() override {
            // 虚拟实现：模拟运行周期
            // 在实际实现中，这里会执行伺服控制循环
        }
        
        virtual void emergStop(void) override {
            // 虚拟实现：紧急停止
            velocity_ = 0.0;
            acceleration_ = 0.0;
            powerStatus_ = false;
        }
    };
}

#endif
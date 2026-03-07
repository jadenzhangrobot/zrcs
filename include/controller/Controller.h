#ifndef CONTROLLER
#define CONTROLLER
#include <fstream>
#include <memory>
#include <vector>
#include "axisConfig.h"
#include "ControllerInterface.h"
#ifdef REALTIME 
#include "ethercat/EthercatMaster.h"
#include "controller/rtos/xenomai.h"
#include "ethercat/EthercatMotor.h"
#endif
#include <controller/virtual/virtualServo.h>
#include <controller/virtual/Coppeliasim.h>
#include "controller/rtos/linux.h"
#include "common/rtLog.h"

namespace ZrcsHardware {
   
class Controller {  
private:        
    AxisConfig *axisConfig_;
    
public:
    // 禁用拷贝构造和赋值
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
    
#ifdef REALTIME 
    EthercatMaster* ethercatMaster_;
    Controller() : axisConfig_(new AxisConfig("axis.xml")), 
                   ethercatMaster_(new EthercatMaster())
#else 
    Controller() : axisConfig_(new AxisConfig("axis.xml"))
#endif
    {
        for(auto it = axisConfig_->axisParas.begin(); it != axisConfig_->axisParas.end(); ++it)
        {     
#ifdef REALTIME 
            if (it->axisId == axiss.size()) 
            {
                Axis* axis = new Axis(it->axisId, it->slaveId, &*it);
                axis->pushServo(new EthercatMotor(it->slaveId, ethercatMaster_));
                axiss.push_back(axis);
            }
            else 
            {
                axiss[it->axisId]->pushServo(new EthercatMotor(it->slaveId, ethercatMaster_));
            }                                 
#endif
            
#ifdef SIMULATION
            if (it->axisId == axiss.size()) 
            {
                Axis* axis = new Axis(it->axisId, it->slaveId, &*it);
                axis->pushServo(new Coppeliasim(it->slaveId));
                axiss.push_back(axis);
            }
            else 
            {
                axiss[it->axisId]->pushServo(new Coppeliasim(it->slaveId));
            }
#endif                                                                
            
#ifdef STANDARD
            if (it->axisId == axiss.size()) 
            {
                Axis* axis = new Axis(it->axisId, it->slaveId, &*it);
                axis->pushServo(new virtualServo(it->slaveId));
                axiss.push_back(axis);
            }
            else 
            {
                axiss[it->axisId]->pushServo(new virtualServo(it->slaveId));
            }            
#endif
        }
        
#ifdef REALTIME 
        rtos_.reset(static_cast<ZrcsHardware::Rtos*>(new xenomai()));            
#else
        rtos_.reset(static_cast<ZrcsHardware::Rtos*>(new Nativelinux()));
#endif                  
    }
    
    void sendData()
    {             
        for(auto it = axiss.begin(); it != axiss.end(); ++it)
        {
            (*it)->updateMotionCmdsToServo();
        }
#ifdef REALTIME 
        ethercatMaster_->send();
#endif
    }
    
    void receiveData()
    {      
#ifdef REALTIME 
        ethercatMaster_->receive();
#endif
        for(auto it = axiss.begin(); it != axiss.end(); ++it)
        {
            (*it)->statusSync();
            (*it)->cyclerun();
        }
    }
    
    void readIo()
    {
    }
    
    void writeIo()
    {
    } 
    
    ~Controller()
    {
        delete axisConfig_;
#ifdef REALTIME 
        delete ethercatMaster_;
#endif
        for (auto ptr : axiss) 
        {
            delete ptr;
        }
        axiss.clear();

        for (auto ptr : ios_)
        {
            delete ptr;
        }
        ios_.clear();
    }
    
    std::vector<uint8_t> outputData_;
    std::vector<uint8_t> inputData_;
    std::shared_ptr<Rtos> rtos_;
    std::vector<Axis*> axiss;
    std::vector<Io*> ios_;
};

}
#endif

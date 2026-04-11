#pragma once

#include <fstream>
#include <memory>
#include <vector>
#include "AxisConfig.h"
#include "ControllerInterface.h"
#include "laser/LaserController.h"
#include "system/RtLog.h"
#include "shared_memory/ShmLayout.h"

namespace ZrcsHardware {

class Controller {
private:
    std::unique_ptr<AxisConfig> axisConfig_;
    std::unique_ptr<HardwareBus> hardwareBus_;
    std::unique_ptr<LaserController> laser_;
    
public:
    // 禁用拷贝构造和赋值
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
    
    // 依赖注入构造函数
    Controller(std::unique_ptr<AxisConfig> config, 
               std::shared_ptr<Rtos> rtos,
               std::unique_ptr<HardwareBus> bus = nullptr) 
        : axisConfig_(std::move(config)), 
          rtos_(rtos),
          hardwareBus_(std::move(bus))
    {
    }

    void addAxis(std::unique_ptr<Axis> axis)
    {
        axiss.push_back(std::move(axis));
    }

    void addIo(std::unique_ptr<Io> io)
    {
        ios_.push_back(std::move(io));
    }

    void setLaser(std::unique_ptr<LaserController> laser)
    {
        laser_ = std::move(laser);
    }

    LaserController* laser() const { return laser_.get(); }
    
    void sendData();

    void receiveData();
    
    void readIo()
    {
    }
    
    void writeIo()
    {
    } 
    
    ~Controller() = default;
    
    std::vector<uint8_t> outputData_;
    std::vector<uint8_t> inputData_;
    std::shared_ptr<Rtos> rtos_;
    std::vector<std::unique_ptr<Axis>> axiss;
    std::vector<std::unique_ptr<Io>> ios_;
};

}

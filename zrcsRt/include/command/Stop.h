#pragma once
#include "Cmdhead.h"
#include "../controller/ControllerInterface.h"

class Stop : public Command {
public:
    explicit Stop(Motor& motor) : motor_(motor) {}
    
    void execute() override {
        motor_.disable();
        motor_.setTargetToq(0.0);  // 确保力矩归零
    }

private:
    Motor& motor_;
};

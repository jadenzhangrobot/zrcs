#pragma once

#include "xml/XmlParsing.h"
#include "config/Parameter.h"
#include "Global.h"
#include <boost/container/options.hpp>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
using namespace tinyxml2;
namespace ZrcsHardware 
{ 
        typedef struct {
        uint32_t axisId;
        uint32_t slaveId;
        std::string axisName;
        MC_SERVO_CONTROL_MODE mode;
        uint64_t encoderCountPerUnit;
        double maxVel;
        double maxAcc;
        double maxJerk;
        double posPositiveLimit;
        double posNegativeLimit;
        double maxPosDiff;
        double frequency;
    }AxisPara;
    class AxisConfig:private XmlParsing
    {
            public:
            std::vector<AxisPara> axisParas;
            AxisConfig(const std::string& xmlFileName);
    };
}

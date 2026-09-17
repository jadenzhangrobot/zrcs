#pragma once

#include "controller/Servo.h"
#include "controller/mujoco/MujocoConfig.h"
#include "shared_memory/ShmLayout.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ZrcsHardware {

class MujocoSimulation {
public:
    explicit MujocoSimulation(const MujocoConfig& config);
    ~MujocoSimulation();

    MujocoSimulation(const MujocoSimulation&) = delete;
    MujocoSimulation& operator=(const MujocoSimulation&) = delete;

    void setTargetPosition(uint32_t slaveId, double position);
    void setTargetVelocity(uint32_t slaveId, double velocity);
    void setTargetTorque(uint32_t slaveId, double torque);
    void setMode(uint32_t slaveId, Cia402Mode mode);
    void setEnabled(uint32_t slaveId, bool enabled);
    void holdCurrentPosition(uint32_t slaveId);
    void setAxisId(uint32_t slaveId, uint32_t axisId);

    double position(uint32_t slaveId) const;
    double velocity(uint32_t slaveId) const;
    double acceleration(uint32_t slaveId) const;
    double appliedTorque(uint32_t slaveId) const;
    double inverseTorque(uint32_t slaveId) const;
    double ctrl(uint32_t slaveId) const;
    bool saturated(uint32_t slaveId) const;
    double time() const;

    void step();
    void receive();
    void applyParamUpdate(const zrcs::MujocoParamUpdate& update);
    void fillIdentificationSample(zrcs::MujocoIdentSampleData& sample,
                                  const zrcs::MujocoIdentCommandData* command,
                                  double timestampSec) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ZrcsHardware

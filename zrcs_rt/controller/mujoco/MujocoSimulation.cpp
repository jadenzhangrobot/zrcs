#include "controller/mujoco/MujocoSimulation.h"

#include <stdexcept>

#ifdef ZRCS_HAS_MUJOCO

#include <mujoco/mujoco.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ZrcsHardware {
namespace {

struct MjModelDeleter {
    void operator()(mjModel* model) const
    {
        if (model) {
            mj_deleteModel(model);
        }
    }
};

struct MjDataDeleter {
    void operator()(mjData* data) const
    {
        if (data) {
            mj_deleteData(data);
        }
    }
};

class ModelHandle {
public:
    explicit ModelHandle(const std::filesystem::path& modelPath)
    {
        std::array<char, 1024> error{};
        mjModel* raw = mj_loadXML(modelPath.string().c_str(), nullptr,
                                  error.data(), static_cast<int>(error.size()));
        if (!raw) {
            throw std::runtime_error("Failed to load MuJoCo model " +
                                     modelPath.string() + ": " + error.data());
        }
        model_.reset(raw);
    }

    mjModel* get() const { return model_.get(); }

private:
    std::unique_ptr<mjModel, MjModelDeleter> model_;
};

struct BindingRuntime {
    MujocoServoConfig config;
    int jointId = -1;
    int qposAdr = -1;
    int dofAdr = -1;
    int actuatorId = -1;
    bool forceLimited = false;
    double forceMin = 0.0;
    double forceMax = 0.0;
    bool ctrlLimited = false;
    double ctrlMin = 0.0;
    double ctrlMax = 0.0;
    double targetPosition = 0.0;
    double targetVelocity = 0.0;
    double targetTorque = 0.0;
    bool enabled = true;
    Cia402Mode mode = Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION;
    uint32_t axisId = 0;
    double lastAppliedTorque = 0.0;
    double lastCtrl = 0.0;
    bool lastSaturated = false;
};

} // namespace

struct MujocoSimulation::Impl {
    explicit Impl(const MujocoConfig& configIn)
        : config(configIn),
          modelHandle(configIn.modelPath),
          model(modelHandle.get())
    {
        data.reset(mj_makeData(model));
        if (!data) {
            throw std::runtime_error("Failed to allocate MuJoCo data");
        }
        inverseData.reset(mj_makeData(model));
        if (!inverseData) {
            throw std::runtime_error("Failed to allocate MuJoCo inverse data");
        }

        model->opt.timestep = (config.timestepMs * 0.001) /
                              static_cast<double>(config.substeps);

        for (const auto& servo : config.servos) {
            BindingRuntime runtime;
            runtime.config = servo;
            runtime.jointId = mj_name2id(model, mjOBJ_JOINT, servo.joint.c_str());
            if (runtime.jointId < 0) {
                throw std::runtime_error("MuJoCo joint not found for slaveId " +
                                         std::to_string(servo.slaveId) + ": " +
                                         servo.joint);
            }

            const int jointType = model->jnt_type[runtime.jointId];
            if (jointType != mjJNT_HINGE && jointType != mjJNT_SLIDE) {
                throw std::runtime_error("MuJoCo joint must be hinge or slide for slaveId " +
                                         std::to_string(servo.slaveId) + ": " +
                                         servo.joint);
            }

            runtime.qposAdr = model->jnt_qposadr[runtime.jointId];
            runtime.dofAdr = model->jnt_dofadr[runtime.jointId];
            runtime.actuatorId = findJointActuator(runtime.jointId);
            runtime.forceLimited = model->jnt_actfrclimited[runtime.jointId] != 0;
            runtime.forceMin = model->jnt_actfrcrange[2 * runtime.jointId];
            runtime.forceMax = model->jnt_actfrcrange[2 * runtime.jointId + 1];
            if (runtime.actuatorId >= 0) {
                runtime.ctrlLimited = model->actuator_ctrllimited[runtime.actuatorId] != 0;
                runtime.ctrlMin = model->actuator_ctrlrange[2 * runtime.actuatorId];
                runtime.ctrlMax = model->actuator_ctrlrange[2 * runtime.actuatorId + 1];
            }
            runtime.targetPosition = qposToUser(runtime, data->qpos[runtime.qposAdr]);

            order.push_back(servo.slaveId);
            bindings.emplace(servo.slaveId, runtime);
        }

        mj_forward(model, data.get());
    }

    int findJointActuator(int jointId) const
    {
        for (int actuatorId = 0; actuatorId < model->nu; ++actuatorId) {
            if (model->actuator_trntype[actuatorId] == mjTRN_JOINT &&
                model->actuator_trnid[2 * actuatorId] == jointId) {
                return actuatorId;
            }
        }
        return -1;
    }

    BindingRuntime& require(uint32_t slaveId)
    {
        auto it = bindings.find(slaveId);
        if (it == bindings.end()) {
            throw std::runtime_error("MuJoCo slaveId is not bound: " +
                                     std::to_string(slaveId));
        }
        return it->second;
    }

    const BindingRuntime& require(uint32_t slaveId) const
    {
        auto it = bindings.find(slaveId);
        if (it == bindings.end()) {
            throw std::runtime_error("MuJoCo slaveId is not bound: " +
                                     std::to_string(slaveId));
        }
        return it->second;
    }

    double userToQpos(const BindingRuntime& binding, double value) const
    {
        return value * binding.config.qposScale + binding.config.qposOffset;
    }

    double qposToUser(const BindingRuntime& binding, double value) const
    {
        return (value - binding.config.qposOffset) / binding.config.qposScale;
    }

    double qvelToUser(const BindingRuntime& binding, double value) const
    {
        return value / binding.config.qposScale;
    }

    double qaccToUser(const BindingRuntime& binding, double value) const
    {
        return value / binding.config.qposScale;
    }

    void clearAppliedForces()
    {
        std::fill(data->qfrc_applied, data->qfrc_applied + model->nv, 0.0);
    }

    double clampActuatorCtrl(const BindingRuntime& binding, double value) const
    {
        if (!binding.ctrlLimited) {
            return value;
        }
        return std::clamp(value, binding.ctrlMin, binding.ctrlMax);
    }

    double clampJointForce(const BindingRuntime& binding, double value) const
    {
        if (!binding.forceLimited) {
            return value;
        }
        return std::clamp(value, binding.forceMin, binding.forceMax);
    }

    void writePositionActuator(BindingRuntime& binding, double targetQpos)
    {
        const double clamped = clampActuatorCtrl(binding, targetQpos);
        data->ctrl[binding.actuatorId] = clamped;
        binding.lastCtrl = clamped;
        binding.lastSaturated = binding.lastSaturated ||
                                std::abs(clamped - targetQpos) >
                                    10.0 * std::numeric_limits<double>::epsilon();
    }

    void applyManualControl(BindingRuntime& binding,
                            double targetQpos,
                            double targetQvel)
    {
        const double qpos = data->qpos[binding.qposAdr];
        const double qvel = data->qvel[binding.dofAdr];

        double force = 0.0;
        if (binding.mode == Cia402Mode::CYCLIC_SYNCHRONOUS_VELOCITY) {
            force = data->qfrc_bias[binding.dofAdr] +
                    binding.config.kd * (targetQvel - qvel);
        } else if (binding.mode == Cia402Mode::CYCLIC_SYNCHRONOUS_TORQUE) {
            force = binding.targetTorque;
        } else {
            force = data->qfrc_bias[binding.dofAdr] +
                    binding.config.kp * (targetQpos - qpos) +
                    binding.config.kd * (targetQvel - qvel);
        }

        const double clamped = clampJointForce(binding, force);
        data->qfrc_applied[binding.dofAdr] += clamped;
        binding.lastAppliedTorque = clamped;
        binding.lastCtrl = clamped;
        binding.lastSaturated = binding.lastSaturated ||
                                std::abs(clamped - force) >
                                    10.0 * std::numeric_limits<double>::epsilon();
    }

    void applyControls()
    {
        clearAppliedForces();
        for (const auto slaveId : order) {
            auto& binding = require(slaveId);
            binding.lastAppliedTorque = 0.0;
            binding.lastCtrl = 0.0;
            binding.lastSaturated = false;
            const double targetQpos = userToQpos(binding, binding.targetPosition);
            const double targetQvel = binding.targetVelocity * binding.config.qposScale;

            if (!binding.enabled) {
                if (binding.actuatorId >= 0) {
                    writePositionActuator(binding, data->qpos[binding.qposAdr]);
                }
                continue;
            }

            if (binding.actuatorId >= 0 &&
                binding.mode == Cia402Mode::CYCLIC_SYNCHRONOUS_POSITION) {
                writePositionActuator(binding, targetQpos);
                continue;
            }

            if (binding.actuatorId >= 0) {
                writePositionActuator(binding, data->qpos[binding.qposAdr]);
            }
            applyManualControl(binding, targetQpos, targetQvel);
        }
    }

    // 纯运动学：直接写 qpos/qvel，不走 mj_step / 力矩 / PD。
    // 由 mujoco.xml kinematicOnly="true" 启用。
    void applyKinematicState()
    {
        const double dt = model->opt.timestep;
        if (model->nu > 0) {
            std::fill(data->ctrl, data->ctrl + model->nu, 0.0);
        }
        if (model->nv > 0) {
            std::fill(data->qfrc_applied, data->qfrc_applied + model->nv, 0.0);
        }

        for (const auto slaveId : order) {
            auto& binding = require(slaveId);
            binding.lastAppliedTorque = 0.0;
            binding.lastCtrl = 0.0;
            binding.lastSaturated = false;

            const double qOld = data->qpos[binding.qposAdr];
            const double vOld = data->qvel[binding.dofAdr];
            double qNew = qOld;
            double vNew = 0.0;

            if (!binding.enabled) {
                vNew = 0.0;
                qNew = qOld;
            } else if (binding.mode == Cia402Mode::CYCLIC_SYNCHRONOUS_VELOCITY) {
                vNew = binding.targetVelocity * binding.config.qposScale;
                qNew = qOld + vNew * dt;
            } else if (binding.mode == Cia402Mode::CYCLIC_SYNCHRONOUS_TORQUE) {
                vNew = 0.0;
                qNew = qOld;
            } else {
                qNew = userToQpos(binding, binding.targetPosition);
                if (dt > 0.0) {
                    vNew = (qNew - qOld) / dt;
                } else {
                    vNew = binding.targetVelocity * binding.config.qposScale;
                }
            }

            if (model->jnt_limited[binding.jointId]) {
                const double lo = model->jnt_range[2 * binding.jointId];
                const double hi = model->jnt_range[2 * binding.jointId + 1];
                if (qNew < lo) {
                    qNew = lo;
                    vNew = 0.0;
                    binding.lastSaturated = true;
                } else if (qNew > hi) {
                    qNew = hi;
                    vNew = 0.0;
                    binding.lastSaturated = true;
                }
            }

            data->qpos[binding.qposAdr] = qNew;
            data->qvel[binding.dofAdr] = vNew;
            if (dt > 0.0) {
                data->qacc[binding.dofAdr] = (vNew - vOld) / dt;
            } else {
                data->qacc[binding.dofAdr] = 0.0;
            }
            binding.lastCtrl = qNew;
            if (binding.enabled &&
                binding.mode != Cia402Mode::CYCLIC_SYNCHRONOUS_VELOCITY) {
                binding.targetPosition = qposToUser(binding, qNew);
            }
        }
    }

    double actuatorTorque(const BindingRuntime& binding) const
    {
        if (binding.actuatorId < 0 || !data->qfrc_actuator) {
            return 0.0;
        }
        return data->qfrc_actuator[binding.dofAdr];
    }

    double appliedTorque(const BindingRuntime& binding) const
    {
        return actuatorTorque(binding) + data->qfrc_applied[binding.dofAdr];
    }

    void updateInverseData() const
    {
        if (!inverseData) {
            return;
        }
        mj_resetData(model, inverseData.get());
        std::copy(data->qpos, data->qpos + model->nq, inverseData->qpos);
        std::copy(data->qvel, data->qvel + model->nv, inverseData->qvel);
        std::copy(data->qacc, data->qacc + model->nv, inverseData->qacc);
        if (model->nu > 0) {
            std::copy(data->ctrl, data->ctrl + model->nu, inverseData->ctrl);
        }
        mj_inverse(model, inverseData.get());
    }

    double inverseTorque(const BindingRuntime& binding) const
    {
        if (!inverseData) {
            return 0.0;
        }
        return inverseData->qfrc_inverse[binding.dofAdr];
    }

    void applyParamUpdate(const zrcs::MujocoParamUpdate& update)
    {
        if (!update.valid) {
            return;
        }
        BindingRuntime& binding = require(update.slaveId);
        if (update.applyArmature && update.inertia >= 0.0 && std::isfinite(update.inertia)) {
            model->dof_armature[binding.dofAdr] = update.inertia;
        }
        if (update.applyDamping && update.damping >= 0.0 && std::isfinite(update.damping)) {
            model->dof_damping[binding.dofAdr] = update.damping;
        }
        if (update.applyFriction && update.friction >= 0.0 && std::isfinite(update.friction)) {
            model->dof_frictionloss[binding.dofAdr] = update.friction;
        }
        if (update.applyGains && binding.actuatorId >= 0 &&
            update.kp >= 0.0 && update.kv >= 0.0 &&
            std::isfinite(update.kp) && std::isfinite(update.kv)) {
            model->actuator_gainprm[binding.actuatorId * mjNGAIN + 0] = update.kp;
            model->actuator_biasprm[binding.actuatorId * mjNBIAS + 1] = -update.kp;
            model->actuator_biasprm[binding.actuatorId * mjNBIAS + 2] = -update.kv;
        }
        mj_forward(model, data.get());
    }

    void fillIdentificationSample(zrcs::MujocoIdentSampleData& sample,
                                  const zrcs::MujocoIdentCommandData* command,
                                  double timestampSec) const
    {
        sample.timestampSec = timestampSec;
        sample.commandSeq = command ? command->seq : 0;
        sample.sessionId = command ? command->sessionId : 0;
        updateInverseData();

        uint32_t index = 0;
        for (const auto slaveId : order) {
            if (index >= zrcs::kAxisMax) {
                break;
            }
            const auto& binding = require(slaveId);
            sample.axisId[index] = binding.axisId;
            sample.slaveId[index] = slaveId;
            if (command && binding.axisId < zrcs::kAxisMax &&
                (command->validMask & (1ull << binding.axisId)) != 0) {
                sample.qCmd[index] = command->qCmd[binding.axisId];
                sample.dqCmd[index] = command->dqCmd[binding.axisId];
                sample.ddqCmd[index] = command->ddqCmd[binding.axisId];
            } else {
                sample.qCmd[index] = binding.targetPosition;
                sample.dqCmd[index] = binding.targetVelocity;
                sample.ddqCmd[index] = 0.0;
            }
            sample.q[index] = qposToUser(binding, data->qpos[binding.qposAdr]);
            sample.dq[index] = qvelToUser(binding, data->qvel[binding.dofAdr]);
            sample.ddq[index] = qaccToUser(binding, data->qacc[binding.dofAdr]);
            sample.tauApplied[index] = appliedTorque(binding);
            sample.tauInverse[index] = inverseTorque(binding);
            sample.ctrl[index] = binding.lastCtrl;
            sample.saturation[index] = binding.lastSaturated ? 1u : 0u;
            sample.enabled[index] = binding.enabled ? 1u : 0u;
            ++index;
        }
        sample.sampleCount = index;
    }

    MujocoConfig config;
    ModelHandle modelHandle;
    mjModel* model = nullptr;
    std::unique_ptr<mjData, MjDataDeleter> data;
    mutable std::unique_ptr<mjData, MjDataDeleter> inverseData;
    std::unordered_map<uint32_t, BindingRuntime> bindings;
    std::vector<uint32_t> order;
};

MujocoSimulation::MujocoSimulation(const MujocoConfig& config)
    : impl_(std::make_unique<Impl>(config))
{
}

MujocoSimulation::~MujocoSimulation() = default;

void MujocoSimulation::setTargetPosition(uint32_t slaveId, double position)
{
    impl_->require(slaveId).targetPosition = position;
}

void MujocoSimulation::setTargetVelocity(uint32_t slaveId, double velocity)
{
    impl_->require(slaveId).targetVelocity = velocity;
}

void MujocoSimulation::setTargetTorque(uint32_t slaveId, double torque)
{
    impl_->require(slaveId).targetTorque = torque;
}

void MujocoSimulation::setMode(uint32_t slaveId, Cia402Mode mode)
{
    impl_->require(slaveId).mode = mode;
}

void MujocoSimulation::setEnabled(uint32_t slaveId, bool enabled)
{
    auto& binding = impl_->require(slaveId);
    binding.enabled = enabled;
    if (!enabled) {
        binding.targetVelocity = 0.0;
        binding.targetTorque = 0.0;
    }
}

void MujocoSimulation::holdCurrentPosition(uint32_t slaveId)
{
    auto& binding = impl_->require(slaveId);
    binding.targetPosition = impl_->qposToUser(binding, impl_->data->qpos[binding.qposAdr]);
    binding.targetVelocity = 0.0;
}

void MujocoSimulation::setAxisId(uint32_t slaveId, uint32_t axisId)
{
    impl_->require(slaveId).axisId = axisId;
}

double MujocoSimulation::position(uint32_t slaveId) const
{
    const auto& binding = impl_->require(slaveId);
    return impl_->qposToUser(binding, impl_->data->qpos[binding.qposAdr]);
}

double MujocoSimulation::velocity(uint32_t slaveId) const
{
    const auto& binding = impl_->require(slaveId);
    return impl_->qvelToUser(binding, impl_->data->qvel[binding.dofAdr]);
}

double MujocoSimulation::acceleration(uint32_t slaveId) const
{
    const auto& binding = impl_->require(slaveId);
    return impl_->qaccToUser(binding, impl_->data->qacc[binding.dofAdr]);
}

double MujocoSimulation::appliedTorque(uint32_t slaveId) const
{
    const auto& binding = impl_->require(slaveId);
    return impl_->appliedTorque(binding);
}

double MujocoSimulation::inverseTorque(uint32_t slaveId) const
{
    const auto& binding = impl_->require(slaveId);
    impl_->updateInverseData();
    return impl_->inverseTorque(binding);
}

double MujocoSimulation::ctrl(uint32_t slaveId) const
{
    const auto& binding = impl_->require(slaveId);
    return binding.lastCtrl;
}

bool MujocoSimulation::saturated(uint32_t slaveId) const
{
    const auto& binding = impl_->require(slaveId);
    return binding.lastSaturated;
}

double MujocoSimulation::time() const
{
    return impl_->data->time;
}

void MujocoSimulation::step()
{
    for (int i = 0; i < impl_->config.substeps; ++i) {
        if (impl_->config.kinematicOnly) {
            // 运动学：写 qpos → mj_forward（无惯性/力）
            impl_->applyKinematicState();
            mj_forward(impl_->model, impl_->data.get());
            impl_->data->time += impl_->model->opt.timestep;
        } else {
            // 动力学（默认）：PD/执行器 + mj_step
            impl_->applyControls();
            mj_step(impl_->model, impl_->data.get());
        }
    }
}

void MujocoSimulation::receive()
{
    mj_forward(impl_->model, impl_->data.get());
}

void MujocoSimulation::applyParamUpdate(const zrcs::MujocoParamUpdate& update)
{
    impl_->applyParamUpdate(update);
}

void MujocoSimulation::fillIdentificationSample(zrcs::MujocoIdentSampleData& sample,
                                                const zrcs::MujocoIdentCommandData* command,
                                                double timestampSec) const
{
    impl_->fillIdentificationSample(sample, command, timestampSec);
}

} // namespace ZrcsHardware

#else

namespace ZrcsHardware {

struct MujocoSimulation::Impl {};

MujocoSimulation::MujocoSimulation(const MujocoConfig&)
{
    throw std::runtime_error("MuJoCo support was not compiled. Configure with ZRCS_ENABLE_MUJOCO=ON.");
}

MujocoSimulation::~MujocoSimulation() = default;

void MujocoSimulation::setTargetPosition(uint32_t, double) {}
void MujocoSimulation::setTargetVelocity(uint32_t, double) {}
void MujocoSimulation::setTargetTorque(uint32_t, double) {}
void MujocoSimulation::setMode(uint32_t, Cia402Mode) {}
void MujocoSimulation::setEnabled(uint32_t, bool) {}
void MujocoSimulation::holdCurrentPosition(uint32_t) {}
void MujocoSimulation::setAxisId(uint32_t, uint32_t) {}
double MujocoSimulation::position(uint32_t) const { return 0.0; }
double MujocoSimulation::velocity(uint32_t) const { return 0.0; }
double MujocoSimulation::acceleration(uint32_t) const { return 0.0; }
double MujocoSimulation::appliedTorque(uint32_t) const { return 0.0; }
double MujocoSimulation::inverseTorque(uint32_t) const { return 0.0; }
double MujocoSimulation::ctrl(uint32_t) const { return 0.0; }
bool MujocoSimulation::saturated(uint32_t) const { return false; }
double MujocoSimulation::time() const { return 0.0; }
void MujocoSimulation::step() {}
void MujocoSimulation::receive() {}
void MujocoSimulation::applyParamUpdate(const zrcs::MujocoParamUpdate&) {}
void MujocoSimulation::fillIdentificationSample(zrcs::MujocoIdentSampleData&,
                                                const zrcs::MujocoIdentCommandData*,
                                                double) const {}

} // namespace ZrcsHardware

#endif

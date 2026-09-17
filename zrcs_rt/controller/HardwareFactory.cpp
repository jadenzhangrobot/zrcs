/**
 * @file HardwareFactory.cpp
 * @brief HardwareFactory 类方法实现
 *
 * createController() 根据编译宏选择硬件栈:
 * - REALTIME:  创建 xenomai RTOS + EthercatMaster + EthercatMotor/EthercatIo
 * - SIMULATION: 为每个轴添加 virtualServo 虚拟伺服
 * - STANDARD:   为每个轴添加 virtualServo 虚拟伺服
 * - 非 REALTIME 时使用 Nativelinux 作为 RTOS
 *
 * 组装流程: AxisConfig -> Axis -> Servo (pushServo) -> Controller
 */
#include "controller/HardwareFactory.h"

#include "config/ConfigManager.h"
#include <map>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#ifdef REALTIME
#include "controller/ethercat/EthercatMaster.h"
#include "controller/ethercat/EthercatMotor.h"
#include "controller/ethercat/EthercatIo.h"
#include "controller/ethercat/EthercatParameter.h"
#include "controller/rtos/Xenomai.h"
#endif

#if defined(SIMULATION) || defined(STANDARD)
#include "controller/virtual/VirtualServo.h"
#endif

#if defined(SIMULATION)
#include "controller/mujoco/MujocoBus.h"
#include "controller/mujoco/MujocoConfig.h"
#include "controller/mujoco/MujocoServo.h"
#include "controller/mujoco/MujocoSimulation.h"
#endif

#ifndef REALTIME
#include "controller/rtos/Linux.h"
#endif

namespace ZrcsHardware {
namespace {

// HardwareFactory 是文件配置对象到运行时控制器对象之间的桥梁。
// 这里重复做一次模式转换，是因为控制器创建流程不经过旧的 ServoConfig 包装类。
MC_SERVO_CONTROL_MODE parseServoMode(const std::string& mode)
{
    if (mode == "velocity") {
        return MC_SERVO_CONTROL_MODE::mcServoControlModeVelocity;
    }
    if (mode == "torque") {
        return MC_SERVO_CONTROL_MODE::mcServoControlModeTorque;
    }
    if (mode == "position") {
        return MC_SERVO_CONTROL_MODE::mcServoControlModePosition;
    }
    throw std::runtime_error("Unsupported servo mode: " + mode);
}

ServoPara toServoPara(const zrcs::config::ServoConfigData& data)
{
    ServoPara para;
    para.slaveId = data.slaveId;
    para.mode = parseServoMode(data.mode);
    para.encoderCountPerUnit = data.encoderCountPerUnit;
    para.direction = data.direction;
    para.homePos = data.homePos;
    para.posOffset = data.posOffset;
    para.velFactor = data.velFactor;
    return para;
}

#if defined(SIMULATION)
void validateMujocoBindings(const MujocoConfig& mujocoConfig,
                            const std::map<uint32_t, ServoPara>& servoBySlaveId,
                            const std::vector<AxisPara>& axes)
{
    std::unordered_set<uint32_t> axisServoIds;
    for (const auto& axis : axes) {
        for (const auto slaveId : axis.servoSlaveIds) {
            axisServoIds.insert(slaveId);
        }
    }

    std::unordered_set<uint32_t> mujocoServoIds;
    for (const auto& servo : mujocoConfig.servos) {
        mujocoServoIds.insert(servo.slaveId);
        if (servoBySlaveId.find(servo.slaveId) == servoBySlaveId.end()) {
            throw std::runtime_error(
                "mujoco.xml references servo slaveId " +
                std::to_string(servo.slaveId) +
                ", but it does not exist in servo.xml");
        }
        if (axisServoIds.find(servo.slaveId) == axisServoIds.end()) {
            throw std::runtime_error(
                "mujoco.xml references servo slaveId " +
                std::to_string(servo.slaveId) +
                ", but no axis in axis.xml uses it");
        }
    }

    for (const auto slaveId : axisServoIds) {
        if (mujocoServoIds.find(slaveId) == mujocoServoIds.end()) {
            throw std::runtime_error(
                "axis.xml references servo slaveId " +
                std::to_string(slaveId) +
                ", but mujoco.xml does not bind it to a MuJoCo joint");
        }
    }
}
#endif

} // namespace

std::unique_ptr<Controller> HardwareFactory::createController(const std::string& projectName) {
    // projectName 为空时，ConfigManager 会解析 config/project.txt；
    // 随后加载 axis/servo/model 文件，并在创建任何硬件对象前完成跨文件校验。
    auto configManager = zrcs::config::ConfigManager::load(projectName);
    auto config = std::make_unique<AxisConfig>();
    std::map<uint32_t, ServoPara> servoBySlaveId;

    // 建立 slaveId 到伺服参数的索引，因为 axis.xml 只引用伺服 slaveId，
    // 不直接嵌入驱动器参数。
    for (const auto& servoData : configManager.servoConfig().servos) {
        auto servo = toServoPara(servoData);
        servoBySlaveId.emplace(servo.slaveId, servo);
    }

    // 把已经校验过的轴级数据复制到控制器现有的 AxisConfig 容器中。
    // 具体伺服对象要根据当前编译模式决定，所以伺服绑定在下面完成。
    for (const auto& axisData : configManager.axisConfig().axes) {
        AxisPara axis;
        axis.axisId = axisData.axisId;
        axis.axisName = axisData.axisName;
        axis.servoSlaveIds = axisData.servoSlaveIds;
        axis.maxVel = axisData.maxVel;
        axis.maxAcc = axisData.maxAcc;
        axis.maxJerk = axisData.maxJerk;
        axis.posPositiveLimit = axisData.posPositiveLimit;
        axis.posNegativeLimit = axisData.posNegativeLimit;
        axis.maxPosDiff = axisData.maxPosDiff;
        axis.lead = axisData.lead;
        config->axisParas.push_back(axis);
    }
    std::shared_ptr<Rtos> rtos;
    std::unique_ptr<HardwareBus> bus = nullptr;

#ifdef REALTIME
    rtos = std::make_shared<xenomai>();
    {
        // REALTIME 模式下，servo.xml 中的 slaveId 必须对应 ethercat.xml 中的
        // MOTOR 从站。ethercat.xml 与业务配置一样统一由 cereal 解析，
        // SlaveConfig 只把它转成 IgH 需要的运行时结构。
        SlaveConfig slaveConfig(configManager.ethercatPath().string());
        std::map<uint32_t, SlaveConfig::SlaveType> slaveTypes;
        for (const auto& slave : slaveConfig.Slaves) {
            slaveTypes.emplace(slave.SlaveId, slave.slaveType);
        }
        for (const auto& servoData : configManager.servoConfig().servos) {
            const auto slaveIt = slaveTypes.find(servoData.slaveId);
            if (slaveIt == slaveTypes.end() ||
                slaveIt->second != SlaveConfig::SlaveType::MOTOR) {
                throw std::runtime_error("Servo slaveId " +
                                         std::to_string(servoData.slaveId) +
                                         " is not a motor slave in ethercat.xml");
            }
        }
    }
    auto ethercatMaster = std::make_unique<EthercatMaster>(
        configManager.ethercatPath().string());
    void* masterPtr = ethercatMaster.get();
    bus = std::move(ethercatMaster);
#else
    rtos = std::make_shared<Nativelinux>();
    [[maybe_unused]] void* masterPtr = nullptr;
#endif

#if defined(SIMULATION)
    bool useMujoco = false;
    std::shared_ptr<MujocoSimulation> mujocoSimulation;
    if (MujocoConfig::existsInProject(configManager.projectDir())) {
#ifdef ZRCS_HAS_MUJOCO
        const auto mujocoConfig = MujocoConfig::load(configManager.projectDir());
        validateMujocoBindings(mujocoConfig, servoBySlaveId, config->axisParas);
        mujocoSimulation = std::make_shared<MujocoSimulation>(mujocoConfig);
        bus = std::make_unique<MujocoBus>(mujocoSimulation);
        useMujoco = true;
#else
        throw std::runtime_error(
            "mujoco.xml exists in project " + configManager.projectDir().string() +
            ", but MuJoCo support was not compiled. Configure with ZRCS_ENABLE_MUJOCO=ON.");
#endif
    }
#endif

    // 在 config 被 move 之前，先用它创建所有 Axis
    // 在 config 被 move 给 Controller 之前创建所有 Axis。每个 Axis 持有一份
    // AxisPara 拷贝，因为命令执行时会直接从 Axis 对象读取限位等参数。
    std::vector<std::unique_ptr<Axis>> axes;
    for (auto it = config->axisParas.begin(); it != config->axisParas.end(); ++it) {
        if (it->axisId >= axes.size()) {
            axes.resize(static_cast<size_t>(it->axisId) + 1);
        }
        if (!axes[it->axisId]) {
            axes[it->axisId] = std::make_unique<Axis>(it->axisId, new AxisPara(*it));
        }
        Axis* axis = axes[it->axisId].get();
        for (const auto slaveId : it->servoSlaveIds) {
            // ConfigManager 已经校验过这个关系。这里保留本地检查，是为了以后如果
            // HardwareFactory 被手工构造的、未校验配置调用，也能给出清晰错误。
            const auto servoIt = servoBySlaveId.find(slaveId);
            if (servoIt == servoBySlaveId.end()) {
                throw std::runtime_error("Axis references missing servo slaveId " + std::to_string(slaveId));
            }

#ifdef REALTIME
            if (masterPtr) {
                // 实时伺服由 EtherCAT 电机对象和 servo.xml 中的单驱比例/模式共同组成。
                axis->pushServo(std::make_unique<EthercatMotor>(
                    slaveId, static_cast<EthercatMaster*>(masterPtr)), servoIt->second);
            }
#endif

#if defined(SIMULATION)
            if (useMujoco) {
                mujocoSimulation->setAxisId(slaveId, it->axisId);
                axis->pushServo(std::make_unique<MujocoServo>(
                                    mujocoSimulation, slaveId),
                                servoIt->second);
            } else
#endif
#if defined(SIMULATION) || defined(STANDARD)
            {
                // Simulation falls back to VirtualServo when no mujoco.xml exists.
                // Standard mode always keeps the in-process virtual servo.
                axis->pushServo(std::make_unique<virtualServo>(slaveId), servoIt->second);
            }
#endif
        }
    }

    auto controller = std::make_unique<Controller>(std::move(config), rtos, std::move(bus));

    for (auto& axis : axes) {
        if (axis) {
            controller->addAxis(std::move(axis));
        }
    }

#ifdef REALTIME
    // 为 AIO/DIO/LASER 类型从站创建 IO 对象
    // 非电机 EtherCAT 从站创建为 IO 对象；电机从站已经在上面绑定到轴。
    if (masterPtr) {
        auto* ecMaster = static_cast<EthercatMaster*>(masterPtr);
        SlaveConfig slaveConfig(configManager.ethercatPath().string());
        for (const auto& slave : slaveConfig.Slaves) {
            if (slave.slaveType == SlaveConfig::SlaveType::AIO ||
                slave.slaveType == SlaveConfig::SlaveType::DIO ||
                slave.slaveType == SlaveConfig::SlaveType::LASER) {
                controller->addIo(std::make_unique<EthercatIo>(slave.SlaveId, ecMaster));
            }
        }
    }
#endif

    return controller;
}

} // namespace ZrcsHardware

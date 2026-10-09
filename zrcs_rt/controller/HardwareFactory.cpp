/**
 * @file HardwareFactory.cpp
 * @brief HardwareFactory 类方法实现 — 文件配置到控制器对象图的装配
 *
 * createController() 是纯「编排」：共享装配（建索引/建轴/绑伺服）抽成无宏的小函数，
 * 平台差异（RTOS / 总线 / 伺服类型 / IO）收敛到各自只有几行的小函数里，宏只出现在平台边界。
 *
 * 共享：buildServoIndex / buildAxisConfig / buildAxes（ServoFactory 回调决定伺服类型）
 * 平台：makeRtos / makeEthercatBus(+校验) / trySetupMujoco / addEthercatIo
 */
#include "controller/HardwareFactory.h"

#include "config/ConfigManager.h"
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
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

// ── 共享装配（跨平台通用，无 #ifdef）───────────────────────────

// 控制器创建流程不经过旧的 ServoConfig 包装类，这里做一次模式转换：
// servo.xml 的字符串 mode → 运行时枚举。
MC_SERVO_CONTROL_MODE parseServoMode(const std::string& mode)
{
    if (mode == "velocity")
    {
        return MC_SERVO_CONTROL_MODE::mcServoControlModeVelocity;
    }
    if (mode == "torque")
    {
        return MC_SERVO_CONTROL_MODE::mcServoControlModeTorque;
    }
    if (mode == "position")
    {
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

/// 建立 slaveId → 伺服参数索引：axis.xml 只引用 slaveId，不嵌入驱动器参数。
std::map<uint32_t, ServoPara> buildServoIndex(const zrcs::config::ConfigManager& cm)
{
    std::map<uint32_t, ServoPara> index;
    for (const auto& servoData : cm.servoConfig().servos)
    {
        auto para = toServoPara(servoData);
        index.emplace(para.slaveId, para);
    }
    return index;
}

/// 把 axis.xml 的轴级数据复制到运行时 AxisConfig 容器。
std::unique_ptr<AxisConfig> buildAxisConfig(const zrcs::config::ConfigManager& cm)
{
    auto config = std::make_unique<AxisConfig>();
    for (const auto& axisData : cm.axisConfig().axes)
    {
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
    return config;
}

/// 伺服工厂回调：平台 Builder 侧提供，按 slaveId/axisId/ServoPara 创建具体伺服。
using ServoFactory = std::function<std::unique_ptr<Servo>(
    uint32_t slaveId, uint32_t axisId, const ServoPara& config)>;

/// 为每个轴创建 Axis 并绑定伺服；伺服对象由 mkServo 按平台构造。
/// 每个 Axis 持有一份 AxisPara 拷贝，因为命令执行时直接从 Axis 对象读取限位等参数。
std::vector<std::unique_ptr<Axis>> buildAxes(AxisConfig& config,
                                             const std::map<uint32_t, ServoPara>& servoIndex,
                                             const ServoFactory& mkServo)
{
    std::vector<std::unique_ptr<Axis>> axes;
    for (auto it = config.axisParas.begin(); it != config.axisParas.end(); ++it)
    {
        if (it->axisId >= axes.size())
        {
            axes.resize(static_cast<size_t>(it->axisId) + 1);
        }
        if (!axes[it->axisId])
        {
            axes[it->axisId] = std::make_unique<Axis>(it->axisId, new AxisPara(*it));
        }
        Axis* axis = axes[it->axisId].get();
        for (const auto slaveId : it->servoSlaveIds)
        {
            // ConfigManager 已经校验过这个关系。这里保留本地检查，是为了以后如果
            // HardwareFactory 被手工构造的、未校验配置调用，也能给出清晰错误。
            const auto servoIt = servoIndex.find(slaveId);
            if (servoIt == servoIndex.end())
            {
                throw std::runtime_error(
                    "Axis references missing servo slaveId " + std::to_string(slaveId));
            }
            axis->pushServo(mkServo(slaveId, it->axisId, servoIt->second));
        }
    }
    return axes;
}

// ── 平台：RTOS ────────────────────────────────────────────────

#ifdef REALTIME
std::shared_ptr<Rtos> makeRtos() { return std::make_shared<xenomai>(); }
#else
std::shared_ptr<Rtos> makeRtos() { return std::make_shared<Nativelinux>(); }
#endif

// ── 平台：EtherCAT（仅 REALTIME 编译）─────────────────────────

#ifdef REALTIME
/// servo.xml 的 slaveId 必须对应 ethercat.xml 中的 MOTOR 从站。
/// ethercat.xml 与业务配置一样统一由 cereal 解析，SlaveConfig 把它转成 IgH 运行时结构。
void validateEthercatServos(const zrcs::config::ConfigManager& cm)
{
    SlaveConfig slaveConfig(cm.ethercatPath().string());
    std::map<uint32_t, SlaveConfig::SlaveType> slaveTypes;
    for (const auto& slave : slaveConfig.Slaves)
    {
        slaveTypes.emplace(slave.SlaveId, slave.slaveType);
    }
    for (const auto& servoData : cm.servoConfig().servos)
    {
        const auto slaveIt = slaveTypes.find(servoData.slaveId);
        if (slaveIt == slaveTypes.end() ||
            slaveIt->second != SlaveConfig::SlaveType::MOTOR)
        {
            throw std::runtime_error("Servo slaveId " +
                                     std::to_string(servoData.slaveId) +
                                     " is not a motor slave in ethercat.xml");
        }
    }
}

/// 创建并激活 EtherCAT 主站；master 指向总线，供伺服与 IO 绑定。
std::unique_ptr<EthercatMaster> makeEthercatBus(const zrcs::config::ConfigManager& cm,
                                                EthercatMaster*& master)
{
    auto bus = std::make_unique<EthercatMaster>(cm.ethercatPath().string());
    master = bus.get();
    return bus;
}

/// 为 AIO/DIO/LASER 类型从站创建 IO 对象（电机从站已绑定到轴）。
void addEthercatIo(Controller& controller, const zrcs::config::ConfigManager& cm,
                   EthercatMaster* master)
{
    SlaveConfig slaveConfig(cm.ethercatPath().string());
    for (const auto& slave : slaveConfig.Slaves)
    {
        if (slave.slaveType == SlaveConfig::SlaveType::AIO ||
            slave.slaveType == SlaveConfig::SlaveType::DIO ||
            slave.slaveType == SlaveConfig::SlaveType::LASER)
        {
            controller.addIo(std::make_unique<EthercatIo>(slave.SlaveId, master));
        }
    }
}
#endif

// ── 平台：MuJoCo（仅 SIMULATION 编译）─────────────────────────

#if defined(SIMULATION)
void validateMujocoBindings(const MujocoConfig& mujocoConfig,
                            const std::map<uint32_t, ServoPara>& servoBySlaveId,
                            const std::vector<AxisPara>& axes)
{
    std::unordered_set<uint32_t> axisServoIds;
    for (const auto& axis : axes)
    {
        for (const auto slaveId : axis.servoSlaveIds)
        {
            axisServoIds.insert(slaveId);
        }
    }

    std::unordered_set<uint32_t> mujocoServoIds;
    for (const auto& servo : mujocoConfig.servos)
    {
        mujocoServoIds.insert(servo.slaveId);
        if (servoBySlaveId.find(servo.slaveId) == servoBySlaveId.end())
        {
            throw std::runtime_error(
                "mujoco.xml references servo slaveId " +
                std::to_string(servo.slaveId) +
                ", but it does not exist in servo.xml");
        }
        if (axisServoIds.find(servo.slaveId) == axisServoIds.end())
        {
            throw std::runtime_error(
                "mujoco.xml references servo slaveId " +
                std::to_string(servo.slaveId) +
                ", but no axis in axis.xml uses it");
        }
    }

    for (const auto slaveId : axisServoIds)
    {
        if (mujocoServoIds.find(slaveId) == mujocoServoIds.end())
        {
            throw std::runtime_error(
                "axis.xml references servo slaveId " +
                std::to_string(slaveId) +
                ", but mujoco.xml does not bind it to a MuJoCo joint");
        }
    }
}

/// MuJoCo 装配结果：active 为 false 时回退 virtualServo。
struct MujocoSetup {
    std::unique_ptr<MujocoBus>                   bus;
    std::shared_ptr<MujocoSimulation>            sim;
    bool                                         active = false;
};

/// 若项目存在 mujoco.xml 则加载并校验；未编译 MuJoCo 支持时给出明确错误。
MujocoSetup trySetupMujoco(const zrcs::config::ConfigManager& cm,
                           const std::map<uint32_t, ServoPara>& servoBySlaveId,
                           const std::vector<AxisPara>& axisParas)
{
    MujocoSetup result;
    if (!MujocoConfig::existsInProject(cm.projectDir()))
    {
        return result;
    }
#ifdef ZRCS_HAS_MUJOCO
    const auto mujocoConfig = MujocoConfig::load(cm.projectDir());
    validateMujocoBindings(mujocoConfig, servoBySlaveId, axisParas);
    result.sim = std::make_shared<MujocoSimulation>(mujocoConfig);
    result.bus = std::make_unique<MujocoBus>(result.sim);
    result.active = true;
#else
    throw std::runtime_error(
        "mujoco.xml exists in project " + cm.projectDir().string() +
        ", but MuJoCo support was not compiled. Configure with ZRCS_ENABLE_MUJOCO=ON.");
#endif
    return result;
}
#endif

} // namespace

// ── 入口：纯编排 ──────────────────────────────────────────────

std::unique_ptr<Controller> HardwareFactory::createController(const std::string& projectName)
{
    // projectName 为空时，ConfigManager 会解析 config/project.txt；
    // 随后加载 axis/servo/model 文件，并在创建任何硬件对象前完成跨文件校验。
    auto configManager = zrcs::config::ConfigManager::load(projectName);
    auto config = buildAxisConfig(configManager);
    auto servoIndex = buildServoIndex(configManager);
    auto rtos = makeRtos();

    // 总线与伺服工厂按编译模式选取（互斥），宏只出现在这处平台边界。
#ifdef REALTIME
    validateEthercatServos(configManager);
    EthercatMaster* master = nullptr;
    auto bus = makeEthercatBus(configManager, master);
    auto mkServo = [master](uint32_t slaveId, uint32_t /*axisId*/, const ServoPara& p)
    {
        // 实时伺服由 EtherCAT 电机对象和 servo.xml 中的单驱比例/模式共同组成。
        return std::make_unique<EthercatMotor>(slaveId, master, p);
    };
#elif defined(SIMULATION)
    auto mujoco = trySetupMujoco(configManager, servoIndex, config->axisParas);
    std::unique_ptr<MujocoBus> bus = std::move(mujoco.bus);
    auto mkServo = [&mujoco](uint32_t slaveId, uint32_t axisId, const ServoPara& p)
        -> std::unique_ptr<Servo>
    {
        if (mujoco.active)
        {
            mujoco.sim->setAxisId(slaveId, axisId);
            return std::make_unique<MujocoServo>(mujoco.sim, slaveId, p);
        }
        // Simulation falls back to VirtualServo when no mujoco.xml exists.
        return std::make_unique<virtualServo>(slaveId, p);
    };
#else
    // Standard 模式一直使用进程内虚拟伺服，无总线。
    auto mkServo = [](uint32_t slaveId, uint32_t /*axisId*/, const ServoPara& p)
    {
        return std::make_unique<virtualServo>(slaveId, p);
    };
#endif

    auto axes = buildAxes(*config, servoIndex, mkServo);

    // config 已被 move 进 Controller，轴已在上一步建好。
#if defined(REALTIME) || defined(SIMULATION)
    auto controller = std::make_unique<Controller>(std::move(config), rtos, std::move(bus));
#else
    auto controller = std::make_unique<Controller>(std::move(config), rtos);
#endif
    for (auto& axis : axes)
    {
        if (axis)
        {
            controller->addAxis(std::move(axis));
        }
    }

#ifdef REALTIME
    addEthercatIo(*controller, configManager, master);
#endif

    return controller;
}

} // namespace ZrcsHardware

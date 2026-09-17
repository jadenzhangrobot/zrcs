#include "controller/mujoco/MujocoConfig.h"

#include "config/MujocoConfig.h"

#include <stdexcept>
#include <string>
#include <unordered_set>

namespace ZrcsHardware {
namespace {

std::filesystem::path configPathFor(const std::filesystem::path& projectDir)
{
    return projectDir / "mujoco.xml";
}

std::filesystem::path resolvePath(const std::filesystem::path& base,
                                  const std::string& value)
{
    std::filesystem::path path(value);
    if (path.is_relative()) {
        path = base / path;
    }
    return path.lexically_normal();
}

} // namespace

const MujocoServoConfig* MujocoConfig::findServo(uint32_t slaveId) const
{
    for (const auto& servo : servos) {
        if (servo.slaveId == slaveId) {
            return &servo;
        }
    }
    return nullptr;
}

bool MujocoConfig::existsInProject(const std::filesystem::path& projectDir)
{
    return std::filesystem::exists(configPathFor(projectDir));
}

MujocoConfig MujocoConfig::load(const std::filesystem::path& projectDir)
{
    // XML 解析统一走 zrcs_common/config 的 cereal 入口。文件字段齐全（cereal
    // 要求已显式写出全部可选项），这里只负责把数据转成运行时结构并保留校验。
    const auto file = zrcs::config::loadXml<zrcs::config::MujocoConfigFile>(
        configPathFor(projectDir));

    MujocoConfig config;
    config.configPath = configPathFor(projectDir);

    if (file.model.empty()) {
        throw std::runtime_error("mujoco.xml missing required model attribute");
    }
    config.modelPath = resolvePath(projectDir, file.model);
    if (!std::filesystem::exists(config.modelPath)) {
        throw std::runtime_error("MuJoCo model file not found: " +
                                 config.modelPath.string());
    }

    config.timestepMs = file.timestepMs;
    config.substeps = file.substeps;
    config.kinematicOnly = file.kinematicOnly;
    if (config.timestepMs <= 0.0) {
        throw std::runtime_error("mujoco.xml timestepMs must be positive");
    }
    if (config.substeps <= 0) {
        throw std::runtime_error("mujoco.xml substeps must be positive");
    }

    std::unordered_set<uint32_t> seenSlaveIds;
    for (const auto& servoData : file.servos) {
        MujocoServoConfig servo;
        servo.slaveId = servoData.slaveId;

        if (servoData.joint.empty()) {
            throw std::runtime_error("mujoco.xml servo missing required joint attribute");
        }
        servo.joint = servoData.joint;

        servo.kp = servoData.kp;
        servo.kd = servoData.kd;
        servo.qposScale = servoData.qposScale;
        servo.qposOffset = servoData.qposOffset;

        if (!seenSlaveIds.insert(servo.slaveId).second) {
            throw std::runtime_error("Duplicate MuJoCo servo slaveId: " +
                                     std::to_string(servo.slaveId));
        }
        if (servo.qposScale == 0.0) {
            throw std::runtime_error("MuJoCo servo qposScale must not be zero for slaveId " +
                                     std::to_string(servo.slaveId));
        }
        if (servo.kp < 0.0 || servo.kd < 0.0) {
            throw std::runtime_error("MuJoCo servo kp/kd must be non-negative for slaveId " +
                                     std::to_string(servo.slaveId));
        }

        config.servos.push_back(std::move(servo));
    }

    if (config.servos.empty()) {
        throw std::runtime_error("mujoco.xml must contain at least one <servo>");
    }

    return config;
}

} // namespace ZrcsHardware

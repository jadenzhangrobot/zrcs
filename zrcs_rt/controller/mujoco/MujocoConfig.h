#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ZrcsHardware {

struct MujocoServoConfig {
    uint32_t slaveId = 0;
    std::string joint;
    double kp = 200.0;
    double kd = 20.0;
    double qposScale = 1.0;
    double qposOffset = 0.0;
};

struct MujocoConfig {
    std::filesystem::path configPath;
    std::filesystem::path modelPath;
    double timestepMs = 1.0;
    int substeps = 1;
    std::vector<MujocoServoConfig> servos;

    const MujocoServoConfig* findServo(uint32_t slaveId) const;

    static bool existsInProject(const std::filesystem::path& projectDir);
    static MujocoConfig load(const std::filesystem::path& projectDir);
};

} // namespace ZrcsHardware

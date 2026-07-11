#include "controller/mujoco/MujocoConfig.h"

#include "tinyxml2.h"

#include <stdexcept>
#include <string>
#include <unordered_set>

namespace ZrcsHardware {
namespace {

std::filesystem::path configPathFor(const std::filesystem::path& projectDir)
{
    return projectDir / "mujoco.xml";
}

std::string xmlError(const std::filesystem::path& path, tinyxml2::XMLError error)
{
    return path.string() + ": " + tinyxml2::XMLDocument::ErrorIDToName(error);
}

double optionalDouble(tinyxml2::XMLElement* elem, const char* name, double defaultValue)
{
    double value = defaultValue;
    const auto status = elem->QueryDoubleAttribute(name, &value);
    if (status != tinyxml2::XML_SUCCESS && status != tinyxml2::XML_NO_ATTRIBUTE) {
        throw std::runtime_error(std::string("Invalid numeric attribute '") + name + "'");
    }
    return value;
}

int optionalInt(tinyxml2::XMLElement* elem, const char* name, int defaultValue)
{
    int value = defaultValue;
    const auto status = elem->QueryIntAttribute(name, &value);
    if (status != tinyxml2::XML_SUCCESS && status != tinyxml2::XML_NO_ATTRIBUTE) {
        throw std::runtime_error(std::string("Invalid integer attribute '") + name + "'");
    }
    return value;
}

uint32_t requiredUint(tinyxml2::XMLElement* elem, const char* name)
{
    unsigned value = 0;
    const auto status = elem->QueryUnsignedAttribute(name, &value);
    if (status != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error(std::string("Missing or invalid required attribute '") + name + "'");
    }
    return static_cast<uint32_t>(value);
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
    MujocoConfig config;
    config.configPath = configPathFor(projectDir);

    tinyxml2::XMLDocument doc;
    const auto status = doc.LoadFile(config.configPath.string().c_str());
    if (status != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Failed to load mujoco.xml: " +
                                 xmlError(config.configPath, status));
    }

    auto* root = doc.FirstChildElement("mujocoConfig");
    if (!root) {
        throw std::runtime_error("mujoco.xml missing <mujocoConfig> root");
    }

    const char* model = root->Attribute("model");
    if (!model || std::string(model).empty()) {
        throw std::runtime_error("mujoco.xml missing required model attribute");
    }
    config.modelPath = resolvePath(projectDir, model);
    if (!std::filesystem::exists(config.modelPath)) {
        throw std::runtime_error("MuJoCo model file not found: " +
                                 config.modelPath.string());
    }

    config.timestepMs = optionalDouble(root, "timestepMs", config.timestepMs);
    config.substeps = optionalInt(root, "substeps", config.substeps);
    // kinematicOnly: "true"/"1" 开，缺省或其它值为动力学
    if (const char* kin = root->Attribute("kinematicOnly")) {
        const std::string v(kin);
        config.kinematicOnly = (v == "1" || v == "true" || v == "True" || v == "TRUE");
    }
    if (config.timestepMs <= 0.0) {
        throw std::runtime_error("mujoco.xml timestepMs must be positive");
    }
    if (config.substeps <= 0) {
        throw std::runtime_error("mujoco.xml substeps must be positive");
    }

    std::unordered_set<uint32_t> seenSlaveIds;
    for (auto* elem = root->FirstChildElement("servo"); elem;
         elem = elem->NextSiblingElement("servo")) {
        MujocoServoConfig servo;
        servo.slaveId = requiredUint(elem, "slaveId");

        const char* joint = elem->Attribute("joint");
        if (!joint || std::string(joint).empty()) {
            throw std::runtime_error("mujoco.xml servo missing required joint attribute");
        }
        servo.joint = joint;

        servo.kp = optionalDouble(elem, "kp", servo.kp);
        servo.kd = optionalDouble(elem, "kd", servo.kd);
        servo.qposScale = optionalDouble(elem, "qposScale", servo.qposScale);
        servo.qposOffset = optionalDouble(elem, "qposOffset", servo.qposOffset);

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

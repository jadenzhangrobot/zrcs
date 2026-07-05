#include "ConfigManager.h"

#include <cmath>
#include <fstream>
#include <stdexcept>

namespace zrcs::config {
namespace {

// project.txt 刻意保持简单：只保存一个项目名，允许末尾带空白或换行。
// 这里统一 trim，可以兼容 Windows/Linux/GUI 工具的手工编辑结果。
std::string trim(std::string value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

} // namespace

ConfigManager::ConfigManager(const std::string& projectName)
{
    // 构造函数复用静态工厂函数，保证两种入口的加载和校验行为完全一致。
    *this = load(projectName);
}

ConfigManager ConfigManager::load(const std::string& projectName)
{
    ConfigManager manager;
    manager.projectDir_ = resolveProjectDir(projectName);

    // axis.xml 和 servo.xml 是必需文件。没有逻辑轴和驱动绑定关系，
    // HardwareFactory 无法创建控制器。
    manager.axisConfig_ = loadXml<AxisConfigFile>(manager.projectDir_ / "axis.xml");
    manager.servoConfig_ = loadXml<ServoConfigFile>(manager.projectDir_ / "servo.xml");

    // model.xml 对纯轴运动场景是可选的。缺少运动学模型时，模型相关命令不可用，
    // 但基础轴运动仍然可以运行。
    const auto modelPath = manager.projectDir_ / "model.xml";
    if (std::filesystem::exists(modelPath)) {
        manager.modelConfig_ = loadXml<ModelConfigFile>(modelPath);
    }

    manager.validate();
    return manager;
}

const ServoConfigData* ConfigManager::findServo(uint32_t slaveId) const
{
    // 索引在 validate() 中构建，启动阶段查找时不需要线性扫描。
    const auto it = servoBySlaveId_.find(slaveId);
    return it == servoBySlaveId_.end() ? nullptr : it->second;
}

const AxisConfigData* ConfigManager::findAxis(uint32_t axisId) const
{
    // 索引在 validate() 中构建。返回 nullptr 让调用方按自己的上下文决定是否致命。
    const auto it = axisByAxisId_.find(axisId);
    return it == axisByAxisId_.end() ? nullptr : it->second;
}

std::filesystem::path ConfigManager::configRoot()
{
    auto current = std::filesystem::current_path();
    while (!current.empty()) {
        // 进程可能从 build/bin、build/ 或仓库根目录启动。向上查找 config 目录，
        // 可以让脚本启动和 IDE 启动使用同一套项目配置定位规则。
        auto candidate = current / "config";
        if (std::filesystem::exists(candidate) && std::filesystem::is_directory(candidate)) {
            return candidate;
        }
        if (current == current.root_path()) {
            break;
        }
        current = current.parent_path();
    }
    throw std::runtime_error("Unable to locate config directory from " +
                             std::filesystem::current_path().string());
}

std::filesystem::path ConfigManager::resolveProjectDir(const std::string& projectName)
{
    auto root = configRoot();
    std::string resolved = trim(projectName);
    if (resolved.empty()) {
        // 空项目名表示“使用当前选中的项目”。
        std::ifstream ifs(root / "project.txt");
        if (ifs.is_open()) {
            std::getline(ifs, resolved);
            resolved = trim(resolved);
        }
    }
    const auto dir = resolved.empty() ? root : (root / resolved);
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        throw std::runtime_error("Config project directory not found: " + dir.string());
    }
    return dir;
}

void ConfigManager::validate()
{
    servoBySlaveId_.clear();
    axisByAxisId_.clear();

    // 伺服 slaveId 必须全局唯一：REALTIME 模式下它一一对应 EtherCAT 从站，
    // 其他模式下它对应虚拟驱动器 ID。
    for (const auto& servo : servoConfig_.servos) {
        if (!servoBySlaveId_.emplace(servo.slaveId, &servo).second) {
            throw std::runtime_error("Duplicate servo slaveId: " + std::to_string(servo.slaveId));
        }
        if (servo.encoderCountPerUnit == 0) {
            throw std::runtime_error("Servo slaveId " + std::to_string(servo.slaveId) +
                                     " has zero encoder posFactor");
        }
        if (servo.direction != 1 && servo.direction != -1) {
            throw std::runtime_error("Servo slaveId " + std::to_string(servo.slaveId) +
                                     " has invalid direction, expected 1 or -1");
        }
    }

    // axisId 是面向用户和命令层的逻辑轴 ID。一个伺服只能属于一个轴；
    // 如果同一个 slaveId 被多个轴引用，就会出现两个逻辑轴同时命令同一驱动器。
    std::set<uint32_t> usedServoIds;
    for (const auto& axis : axisConfig_.axes) {
        if (!axisByAxisId_.emplace(axis.axisId, &axis).second) {
            throw std::runtime_error("Duplicate axisId: " + std::to_string(axis.axisId));
        }
        if (!std::isfinite(axis.lead) || axis.lead <= 0.0) {
            throw std::runtime_error("Axis " + std::to_string(axis.axisId) +
                                     " has invalid lead, expected > 0");
        }
        if (axis.servoSlaveIds.empty()) {
            throw std::runtime_error("Axis " + std::to_string(axis.axisId) +
                                     " does not reference any servo");
        }
        for (const auto slaveId : axis.servoSlaveIds) {
            if (servoBySlaveId_.find(slaveId) == servoBySlaveId_.end()) {
                throw std::runtime_error("Axis " + std::to_string(axis.axisId) +
                                         " references missing servo slaveId " +
                                         std::to_string(slaveId));
            }
            if (!usedServoIds.insert(slaveId).second) {
                throw std::runtime_error("Servo slaveId " + std::to_string(slaveId) +
                                         " is referenced by multiple axes or duplicated");
            }
        }
    }

    // model.xml 中的关节会引用 axis.xml 中的逻辑轴。这里在轴索引建立后统一校验，
    // 避免轨迹/模型命令执行到一半才发现关节引用了不存在的轴。
    for (const auto& model : modelConfig_.models) {
        if (model.dof != static_cast<int>(model.joints.size())) {
            throw std::runtime_error("Model " + model.name + " dof does not match joints.size()");
        }
        for (const auto& joint : model.joints) {
            if (axisByAxisId_.find(static_cast<uint32_t>(joint.axisId)) == axisByAxisId_.end()) {
                throw std::runtime_error("Model " + model.name +
                                         " references missing axisId " +
                                         std::to_string(joint.axisId));
            }
        }
    }
}

} // namespace zrcs::config

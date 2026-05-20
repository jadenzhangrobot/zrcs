#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>

#include "ConfigSerializer.h"

namespace zrcs::config {

/**
 * @brief 管理一个项目加载完成后的全部配置。
 *
 * ConfigManager 是磁盘配置文件和运行时代码之间的统一边界。它负责解析当前
 * 项目目录，把 axis.xml、servo.xml、model.xml 加载成强类型对象，并统一校验
 * 文件之间的引用关系。外部代码应优先使用这里的强类型访问接口，而不是重复打开
 * 配置文件。
 */
class ConfigManager {
public:
    ConfigManager() = default;

    /**
     * @brief 构造时立即加载项目配置。
     *
     * projectName 为空时读取 config/project.txt；传入项目名时读取
     * config/<projectName>/。
     */
    explicit ConfigManager(const std::string& projectName);

    /**
     * @brief 创建一个已经加载并完成校验的项目配置管理器。
     *
     * @throws std::runtime_error 文件缺失、XML 格式错误或跨文件引用非法时抛出。
     */
    static ConfigManager load(const std::string& projectName);

    /// 当前选中项目在 config/ 下的绝对路径。
    const std::filesystem::path& projectDir() const { return projectDir_; }

    /// EtherCAT 继续由 SlaveConfig 使用 tinyxml2 解析，因此这里只提供路径。
    std::filesystem::path ethercatPath() const { return projectDir_ / "ethercat.xml"; }

    /// 完成校验后的 axis.xml 强类型数据。
    const AxisConfigFile& axisConfig() const { return axisConfig_; }

    /// 完成校验后的 servo.xml 强类型数据。
    const ServoConfigFile& servoConfig() const { return servoConfig_; }

    /// 完成校验后的 model.xml 强类型数据。model.xml 缺失时可以为空。
    const ModelConfigFile& modelConfig() const { return modelConfig_; }

    /// 按 EtherCAT/虚拟伺服 slaveId 查找伺服配置，找不到返回 nullptr。
    const ServoConfigData* findServo(uint32_t slaveId) const;

    /// 按逻辑 axisId 查找轴配置，找不到返回 nullptr。
    const AxisConfigData* findAxis(uint32_t axisId) const;

private:
    std::filesystem::path projectDir_;
    AxisConfigFile axisConfig_;
    ServoConfigFile servoConfig_;
    ModelConfigFile modelConfig_;

    // 这些索引里的指针指向 axisConfig_/servoConfig_ 内部对象。
    // 每次加载后 validate() 会重新构建索引，再把 manager 返回给调用方。
    std::map<uint32_t, const ServoConfigData*> servoBySlaveId_;
    std::map<uint32_t, const AxisConfigData*> axisByAxisId_;

    /// 从 current_path() 开始向上查找，直到找到 config 目录。
    static std::filesystem::path configRoot();

    /// 根据显式项目名或 config/project.txt 解析项目目录。
    static std::filesystem::path resolveProjectDir(const std::string& projectName);

    /// 校验唯一性，以及 axis/servo/model 之间的所有跨文件引用。
    void validate();
};

} // namespace zrcs::config

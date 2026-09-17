#pragma once

#include <filesystem>
#include <string>

#include "AxisConfig.h"
#include "ModelConfig.h"
#include "ServoConfig.h"

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
     * @brief 创建一个已经加载并完成校验的项目配置管理器。
     *
     * 解析当前项目目录并把 axis.xml、servo.xml、model.xml 加载成强类型对象，
     * 统一校验文件之间的引用关系。
     *
     * @throws std::runtime_error 文件缺失、XML 格式错误或跨文件引用非法时抛出。
     */
    static ConfigManager load(const std::string& projectName);

    /// 当前选中项目在 config/ 下的绝对路径。
    const std::filesystem::path& projectDir() const { return projectDir_; }

    /// EtherCAT 拓扑与 PDO 布局由同样走 cereal 的 EthercatConfigFile 解析，
    /// 这里只提供文件路径，SlaveConfig 负责把它转成 IgH 运行时结构。
    std::filesystem::path ethercatPath() const { return projectDir_ / "ethercat.xml"; }

    /// 完成校验后的 axis.xml 强类型数据。
    const AxisConfigFile& axisConfig() const { return axisConfig_; }

    /// 完成校验后的 servo.xml 强类型数据。
    const ServoConfigFile& servoConfig() const { return servoConfig_; }

    /// 完成校验后的 model.xml 强类型数据。model.xml 缺失时可以为空。
    const ModelConfigFile& modelConfig() const { return modelConfig_; }

private:
    std::filesystem::path projectDir_;
    AxisConfigFile axisConfig_;
    ServoConfigFile servoConfig_;
    ModelConfigFile modelConfig_;

    /// 从 current_path() 开始向上查找，直到找到 config 目录。
    static std::filesystem::path configRoot();

    /// 根据显式项目名或 config/project.txt 解析项目目录。
    static std::filesystem::path resolveProjectDir(const std::string& projectName);

    /// 校验唯一性，以及 axis/servo/model 之间的所有跨文件引用。
    void validate();
};

} // namespace zrcs::config

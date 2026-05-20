/**
 * @file ProjectConfig.h
 * @brief 基于项目目录的配置选择工具。
 * @details 从 config/project.txt 读取当前项目名，并提供项目内配置文件路径拼接。
 */
#pragma once
#include <string>

namespace zrcs {

class ProjectConfig {
public:
    /// 从 config/project.txt 读取项目名；文件缺失、为空或不可读时返回空字符串。
    static std::string resolve();

    /// 给配置文件名加上项目子目录前缀。
    /// prefixedFilename("ur5", "axis.xml") -> "ur5/axis.xml"
    /// prefixedFilename("",    "axis.xml") -> "axis.xml"
    static std::string prefixedFilename(const std::string& project,
                                         const std::string& filename);
};

} // namespace zrcs

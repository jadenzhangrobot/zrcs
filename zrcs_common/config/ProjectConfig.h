/**
 * @file ProjectConfig.h
 * @brief Project-based configuration selector
 * @details Reads the active project name from config/project.txt,
 *          then provides filename prefixing for per-project config directories.
 */
#pragma once
#include <string>

namespace zrcs {

class ProjectConfig {
public:
    /// Read project name from config/project.txt (same path resolution as XmlParsing).
    /// Returns "" if file is missing, empty, or unreadable.
    static std::string resolve();

    /// Prefix a config filename with the project subdirectory.
    /// prefixedFilename("ur5", "axis.xml") -> "ur5/axis.xml"
    /// prefixedFilename("",    "axis.xml") -> "axis.xml"
    static std::string prefixedFilename(const std::string& project,
                                         const std::string& filename);
};

} // namespace zrcs

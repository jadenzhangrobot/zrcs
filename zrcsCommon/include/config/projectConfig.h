/**
 * @file projectConfig.h
 * @brief Project-based configuration selector
 * @details Reads the active project name from config/project.txt,
 *          then provides filename prefixing for per-project config directories.
 */
#pragma once
#include <string>
#include <fstream>
#include <filesystem>

namespace zrcs {

class ProjectConfig {
public:
    /// Read project name from config/project.txt (same path resolution as XmlParsing).
    /// Returns "" if file is missing, empty, or unreadable.
    static std::string resolve()
    {
        std::string cwd = std::filesystem::current_path().string();
        size_t pos = cwd.find("build");
        if (pos == std::string::npos) return "";

        std::string projectRoot = cwd.substr(0, pos);
        std::string filePath = projectRoot + "config/project.txt";

        std::ifstream ifs(filePath);
        if (!ifs.is_open()) return "";

        std::string line;
        std::getline(ifs, line);
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        return line;
    }

    /// Prefix a config filename with the project subdirectory.
    /// prefixedFilename("ur5", "axis.xml") -> "ur5/axis.xml"
    /// prefixedFilename("",    "axis.xml") -> "axis.xml"
    static std::string prefixedFilename(const std::string& project,
                                         const std::string& filename)
    {
        if (project.empty()) return filename;
        return project + "/" + filename;
    }
};

} // namespace zrcs

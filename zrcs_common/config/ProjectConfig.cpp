#include "ProjectConfig.h"

#include <fstream>
#include <filesystem>

namespace zrcs {

std::string ProjectConfig::resolve()
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

std::string ProjectConfig::prefixedFilename(const std::string& project,
                                              const std::string& filename)
{
    if (project.empty()) return filename;
    return project + "/" + filename;
}

} // namespace zrcs

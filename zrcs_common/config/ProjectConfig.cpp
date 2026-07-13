#include "ProjectConfig.h"

#include <fstream>
#include <filesystem>

namespace zrcs {
namespace {

namespace fs = std::filesystem;

bool isAbsolutePath(const std::string& path)
{
    if (path.empty()) {
        return false;
    }
    // Unix absolute
    if (path[0] == '/' || path[0] == '\\') {
        return true;
    }
    // Windows drive: C:\ or C:/
    if (path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) &&
        path[1] == ':') {
        return true;
    }
    return false;
}

} // namespace

std::string ProjectConfig::projectRoot()
{
    const fs::path cwd = fs::current_path();
    const std::string cwdStr = cwd.string();

    // 常见：在 build/ 或 build/bin 下运行
    const size_t pos = cwdStr.find("build");
    if (pos != std::string::npos && pos > 0) {
        const fs::path root = fs::path(cwdStr.substr(0, pos));
        if (fs::exists(root / "config")) {
            return root.string();
        }
    }

    // 向上查找含 config/ 的目录
    fs::path p = cwd;
    for (int i = 0; i < 8; ++i) {
        if (fs::exists(p / "config")) {
            return p.string();
        }
        if (!p.has_parent_path() || p == p.parent_path()) {
            break;
        }
        p = p.parent_path();
    }
    return {};
}

std::string ProjectConfig::resolve()
{
    const std::string root = projectRoot();
    if (root.empty()) {
        return {};
    }

    const fs::path filePath = fs::path(root) / "config" / "project.txt";
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        return {};
    }

    std::string line;
    std::getline(ifs, line);
    // 去掉 project.txt 中可能由手工编辑留下的首尾空白。
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    if (!line.empty()) {
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
    }
    return line;
}

std::string ProjectConfig::prefixedFilename(const std::string& project,
                                              const std::string& filename)
{
    if (project.empty()) {
        return filename;
    }
    return project + "/" + filename;
}

std::string ProjectConfig::resolvePath(const std::string& path)
{
    if (path.empty()) {
        return path;
    }
    if (isAbsolutePath(path)) {
        return path;
    }

    const fs::path rel(path);
    // 1) 相对当前工作目录
    {
        std::error_code ec;
        const fs::path abs = fs::absolute(rel, ec);
        if (!ec && fs::exists(abs)) {
            return abs.string();
        }
    }
    // 2) 相对工程根
    const std::string root = projectRoot();
    if (!root.empty()) {
        const fs::path underRoot = fs::path(root) / rel;
        if (fs::exists(underRoot)) {
            return underRoot.string();
        }
        // 即使尚不存在也返回拼好的路径，便于错误信息带绝对路径
        return underRoot.string();
    }
    return path;
}

} // namespace zrcs

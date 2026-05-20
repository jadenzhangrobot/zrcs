#include "config/ConfigManager.h"
#include "config/ConfigSerializer.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {

// 测试会在临时目录下创建一次性的 config/ 项目，然后把 current_path()
// 切到类似 build/ 的目录。这样可以覆盖 ConfigManager 向上查找 config 目录的
// 行为，而不是依赖开发者当前 shell 所在路径。
void writeText(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream os(path);
    os << text;
}

void writeProject(const std::filesystem::path& root,
                  const std::string& project,
                  const std::string& axisBody,
                  const std::string& servoBody,
                  const std::string& modelBody)
{
    // 这些 fixture 使用旧的属性风格 XML。这样真实项目文件迁移到 cereal XML 后，
    // 旧格式回退解析仍然有测试覆盖。
    const auto dir = root / "config" / project;
    writeText(dir / "axis.xml",
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?><axisConfig>" +
              axisBody + "</axisConfig>");
    writeText(dir / "servo.xml",
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?><servoConfig>" +
              servoBody + "</servoConfig>");
    writeText(dir / "model.xml",
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?><modelConfig>" +
              modelBody + "</modelConfig>");
}

bool loadFails(const std::string& project)
{
    // 负向校验用 assert(loadFails(...)) 会更直观。
    try {
        (void)zrcs::config::ConfigManager::load(project);
        return false;
    } catch (const std::exception&) {
        return true;
    }
}

} // namespace

int main()
{
    const auto original = std::filesystem::current_path();
    const auto temp = std::filesystem::temp_directory_path() / "zrcs_config_manager_test";
    std::filesystem::remove_all(temp);
    std::filesystem::create_directories(temp / "build");
    std::filesystem::current_path(temp / "build");

    const std::string model =
        "<model name=\"cart\" type=\"cartesian\" dof=\"1\">"
        "<baseFrame x=\"0\" y=\"0\" z=\"0\" rx=\"0\" ry=\"0\" rz=\"0\"/>"
        "<toolFrame x=\"0\" y=\"0\" z=\"0\" rx=\"0\" ry=\"0\" rz=\"0\"/>"
        "<joints><joint axisId=\"0\" type=\"prismatic\" offset=\"0\" axis=\"X\"/></joints>"
        "</model>";

    writeProject(temp, "single",
        "<axis id=\"0\" name=\"x\" maxVel=\"1\" maxAcc=\"2\" maxJerk=\"3\" maxPos=\"10\" minPos=\"-10\" maxPosDiff=\"1\"><servos><servo slaveId=\"0\"/></servos></axis>",
        "<servo slaveId=\"0\" mode=\"position\" posFactor=\"1000\" homePos=\"0\" posOffset=\"0\" velFactor=\"1\"/>",
        model);
    // 单轴单驱是最小有效机器配置。
    auto single = zrcs::config::ConfigManager::load("single");
    assert(single.axisConfig().axes.size() == 1);
    assert(single.servoConfig().servos.size() == 1);

    writeProject(temp, "dual",
        "<axis id=\"0\" name=\"x\" maxVel=\"1\" maxAcc=\"2\" maxJerk=\"3\" maxPos=\"10\" minPos=\"-10\" maxPosDiff=\"1\"><servos><servo slaveId=\"0\"/><servo slaveId=\"1\"/></servos></axis>",
        "<servo slaveId=\"0\" mode=\"position\" posFactor=\"1000\" homePos=\"0\" posOffset=\"0\" velFactor=\"1\"/><servo slaveId=\"1\" mode=\"position\" posFactor=\"1000\" homePos=\"0\" posOffset=\"0\" velFactor=\"1\"/>",
        model);
    // 一个逻辑轴可以拥有两个 slaveId，用于双驱机械结构。
    auto dual = zrcs::config::ConfigManager::load("dual");
    assert(dual.axisConfig().axes.front().servoSlaveIds.size() == 2);

    const auto cerealDir = temp / "config" / "cereal-native";
    // 通过新的 cereal writer 写出后再由 ConfigManager 读回，
    // 覆盖推荐的新格式路径。
    zrcs::config::saveXml(cerealDir / "axis.xml", dual.axisConfig());
    zrcs::config::saveXml(cerealDir / "servo.xml", dual.servoConfig());
    zrcs::config::saveXml(cerealDir / "model.xml", dual.modelConfig());
    auto cerealNative = zrcs::config::ConfigManager::load("cereal-native");
    assert(cerealNative.axisConfig().axes.front().servoSlaveIds.size() == 2);
    assert(cerealNative.servoConfig().servos.size() == 2);

    writeProject(temp, "missing-servo",
        "<axis id=\"0\" name=\"x\" maxVel=\"1\" maxAcc=\"2\" maxJerk=\"3\" maxPos=\"10\" minPos=\"-10\" maxPosDiff=\"1\"><servos><servo slaveId=\"9\"/></servos></axis>",
        "<servo slaveId=\"0\" mode=\"position\" posFactor=\"1000\" homePos=\"0\" posOffset=\"0\" velFactor=\"1\"/>",
        model);
    // axis.xml 不能引用 servo.xml 中不存在的 slaveId。
    assert(loadFails("missing-servo"));

    writeProject(temp, "duplicate-servo-use",
        "<axis id=\"0\" name=\"x\" maxVel=\"1\" maxAcc=\"2\" maxJerk=\"3\" maxPos=\"10\" minPos=\"-10\" maxPosDiff=\"1\"><servos><servo slaveId=\"0\"/></servos></axis>"
        "<axis id=\"1\" name=\"y\" maxVel=\"1\" maxAcc=\"2\" maxJerk=\"3\" maxPos=\"10\" minPos=\"-10\" maxPosDiff=\"1\"><servos><servo slaveId=\"0\"/></servos></axis>",
        "<servo slaveId=\"0\" mode=\"position\" posFactor=\"1000\" homePos=\"0\" posOffset=\"0\" velFactor=\"1\"/>",
        model);
    // 一个驱动器只能属于一个逻辑轴。
    assert(loadFails("duplicate-servo-use"));

    const std::string badModel =
        "<model name=\"bad\" type=\"cartesian\" dof=\"1\">"
        "<baseFrame x=\"0\" y=\"0\" z=\"0\" rx=\"0\" ry=\"0\" rz=\"0\"/>"
        "<toolFrame x=\"0\" y=\"0\" z=\"0\" rx=\"0\" ry=\"0\" rz=\"0\"/>"
        "<joints><joint axisId=\"2\" type=\"prismatic\" offset=\"0\" axis=\"X\"/></joints>"
        "</model>";
    writeProject(temp, "bad-model-axis",
        "<axis id=\"0\" name=\"x\" maxVel=\"1\" maxAcc=\"2\" maxJerk=\"3\" maxPos=\"10\" minPos=\"-10\" maxPosDiff=\"1\"><servos><servo slaveId=\"0\"/></servos></axis>",
        "<servo slaveId=\"0\" mode=\"position\" posFactor=\"1000\" homePos=\"0\" posOffset=\"0\" velFactor=\"1\"/>",
        badModel);
    // model.xml 中的 joint 必须引用已存在的逻辑 axisId。
    assert(loadFails("bad-model-axis"));

    std::filesystem::current_path(original);
    std::filesystem::remove_all(temp);
    return 0;
}

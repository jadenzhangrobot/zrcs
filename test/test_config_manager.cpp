#include "config/ConfigManager.h"
#include "config/ConfigSerializer.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
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

// cereal 原生格式要求每个已序列化字段齐全（缺失会直接解析失败）。下面的
// 生成器保证 fixture 总是完整且可读的。
std::string servoRefs(const std::vector<uint32_t>& slaveIds)
{
    std::ostringstream os;
    int i = 0;
    for (const auto id : slaveIds) {
        os << "<value" << i++ << ">" << id << "</value" << (i - 1) << ">";
    }
    return os.str();
}

std::string axisXml(const uint32_t axisId, const std::vector<uint32_t>& slaveIds)
{
    std::ostringstream os;
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?><cereal><axisConfig>"
       << "<axes size=\"dynamic\">"
       << "<value0><axisId>" << axisId << "</axisId>"
       << "<axisName>axis" << axisId << "</axisName>"
       << "<servoSlaveIds size=\"dynamic\">" << servoRefs(slaveIds) << "</servoSlaveIds>"
       << "<maxVel>1</maxVel><maxAcc>2</maxAcc><maxJerk>3</maxJerk>"
       << "<posPositiveLimit>10</posPositiveLimit><posNegativeLimit>-10</posNegativeLimit>"
       << "<maxPosDiff>1</maxPosDiff><lead>5</lead>"
       << "</value0>"
       << "</axes></axisConfig></cereal>";
    return os.str();
}

std::string servoValueBody(const uint32_t slaveId, const int direction)
{
    std::ostringstream os;
    os << "<value0><slaveId>" << slaveId << "</slaveId>"
       << "<mode>position</mode><encoderCountPerUnit>1000</encoderCountPerUnit>"
       << "<direction>" << direction << "</direction>"
       << "<homePos>0</homePos><posOffset>0</posOffset><velFactor>1</velFactor>"
       << "</value0>";
    return os.str();
}

std::string servoXml(const uint32_t slaveId, const int direction)
{
    return "<?xml version=\"1.0\" encoding=\"UTF-8\"?><cereal><servoConfig>"
           "<servos size=\"dynamic\">" +
           servoValueBody(slaveId, direction) +
           "</servos></servoConfig></cereal>";
}

std::string modelXml(const std::string& axisRef = "0")
{
    std::ostringstream os;
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?><cereal><modelConfig>"
       << "<models size=\"dynamic\">"
       << "<value0><name>cart</name><type>cartesian</type><dof>1</dof>"
       << "<baseFrame><x>0</x><y>0</y><z>0</z><rx>0</rx><ry>0</ry><rz>0</rz></baseFrame>"
       << "<toolFrame><x>0</x><y>0</y><z>0</z><rx>0</rx><ry>0</ry><rz>0</rz></toolFrame>"
       << "<basePlatformRadius>0</basePlatformRadius>"
       << "<mobilePlatformRadius>0</mobilePlatformRadius>"
       << "<upperArmLength>0</upperArmLength><lowerArmLength>0</lowerArmLength>"
       << "<joints size=\"dynamic\">"
       << "<value0><axisId>" << axisRef << "</axisId><type>prismatic</type>"
       << "<offset>0</offset><dh_a>0</dh_a><dh_alpha>0</dh_alpha>"
       << "<dh_d>0</dh_d><dh_theta>0</dh_theta><axis>X</axis></value0>"
       << "</joints></value0>"
       << "</models></modelConfig></cereal>";
    return os.str();
}

void writeProject(const std::filesystem::path& root,
                  const std::string& project,
                  const std::string& axisBody,
                  const std::string& servoBody,
                  const std::string& modelBody)
{
    const auto dir = root / "config" / project;
    writeText(dir / "axis.xml", axisBody);
    writeText(dir / "servo.xml", servoBody);
    writeText(dir / "model.xml", modelBody);
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

    writeProject(temp, "single", axisXml(0, {0}), servoXml(0, 1), modelXml());
    // 单轴单驱是最小有效机器配置。
    auto single = zrcs::config::ConfigManager::load("single");
    assert(single.axisConfig().axes.size() == 1);
    assert(single.axisConfig().axes.front().lead == 5.0);
    assert(single.servoConfig().servos.size() == 1);
    assert(single.servoConfig().servos.front().direction == 1);

    writeProject(temp, "dual",
                std::string("<cereal><axisConfig><axes size=\"dynamic\">") +
                    "<value0><axisId>0</axisId><axisName>x</axisName>" +
                    "<servoSlaveIds size=\"dynamic\"><value0>0</value0><value1>1</value1></servoSlaveIds>" +
                    "<maxVel>1</maxVel><maxAcc>2</maxAcc><maxJerk>3</maxJerk>" +
                    "<posPositiveLimit>10</posPositiveLimit><posNegativeLimit>-10</posNegativeLimit>" +
                    "<maxPosDiff>1</maxPosDiff><lead>5</lead></value0>" +
                    "</axes></axisConfig></cereal>",
                std::string("<cereal><servoConfig><servos size=\"dynamic\">") +
                    servoValueBody(0, 1) + servoValueBody(1, -1) +
                    "</servos></servoConfig></cereal>",
                modelXml());
    // 一个逻辑轴可以拥有两个 slaveId，用于双驱机械结构。
    auto dual = zrcs::config::ConfigManager::load("dual");
    assert(dual.axisConfig().axes.front().servoSlaveIds.size() == 2);
    assert(dual.servoConfig().servos.at(1).direction == -1);

    const auto cerealDir = temp / "config" / "cereal-native";
    // 通过新的 cereal writer 写出后再由 ConfigManager 读回，覆盖推荐路径。
    zrcs::config::saveXml(cerealDir / "axis.xml", dual.axisConfig());
    zrcs::config::saveXml(cerealDir / "servo.xml", dual.servoConfig());
    zrcs::config::saveXml(cerealDir / "model.xml", dual.modelConfig());
    auto cerealNative = zrcs::config::ConfigManager::load("cereal-native");
    assert(cerealNative.axisConfig().axes.front().servoSlaveIds.size() == 2);
    assert(cerealNative.axisConfig().axes.front().lead == 5.0);
    assert(cerealNative.servoConfig().servos.size() == 2);

    writeProject(temp, "missing-servo", axisXml(0, {9}), servoXml(0, 1), modelXml());
    // axis.xml 不能引用 servo.xml 中不存在的 slaveId。
    assert(loadFails("missing-servo"));

    writeProject(temp, "duplicate-servo-use",
                "<cereal><axisConfig><axes size=\"dynamic\">"
                "<value0><axisId>0</axisId><axisName>x</axisName>"
                "<servoSlaveIds size=\"dynamic\"><value0>0</value0></servoSlaveIds>"
                "<maxVel>1</maxVel><maxAcc>2</maxAcc><maxJerk>3</maxJerk>"
                "<posPositiveLimit>10</posPositiveLimit><posNegativeLimit>-10</posNegativeLimit>"
                "<maxPosDiff>1</maxPosDiff><lead>5</lead></value0>"
                "<value1><axisId>1</axisId><axisName>y</axisName>"
                "<servoSlaveIds size=\"dynamic\"><value0>0</value0></servoSlaveIds>"
                "<maxVel>1</maxVel><maxAcc>2</maxAcc><maxJerk>3</maxJerk>"
                "<posPositiveLimit>10</posPositiveLimit><posNegativeLimit>-10</posNegativeLimit>"
                "<maxPosDiff>1</maxPosDiff><lead>5</lead></value1>"
                "</axes></axisConfig></cereal>",
                servoXml(0, 1), modelXml());
    // 一个驱动器只能属于一个逻辑轴。
    assert(loadFails("duplicate-servo-use"));

    writeProject(temp, "bad-model-axis", axisXml(0, {0}), servoXml(0, 1),
                modelXml("2"));
    // model.xml 中的 joint 必须引用已存在的逻辑 axisId。
    assert(loadFails("bad-model-axis"));

    writeProject(temp, "bad-servo-direction", axisXml(0, {0}), servoXml(0, 0),
                modelXml());
    // direction 只能是 1 或 -1，避免配置写错后伺服方向不可预测。
    assert(loadFails("bad-servo-direction"));

    // cereal 原生路径：direction 字段缺失会直接解析失败（不再有旧格式的
    // "缺省方向"兼容）。
    {
        const auto dir = temp / "config" / "missing-servo-direction";
        writeText(dir / "axis.xml", axisXml(0, {0}));
        writeText(dir / "servo.xml",
                  "<?xml version=\"1.0\" encoding=\"UTF-8\"?><cereal><servoConfig>"
                  "<servos size=\"dynamic\">"
                  "<value0><slaveId>0</slaveId><mode>position</mode>"
                  "<encoderCountPerUnit>1000</encoderCountPerUnit>"
                  "<homePos>0</homePos><posOffset>0</posOffset><velFactor>1</velFactor>"
                  "</value0></servos></servoConfig></cereal>");
        writeText(dir / "model.xml", modelXml());
        assert(loadFails("missing-servo-direction"));
    }

    writeProject(temp, "bad-axis-lead",
                "<cereal><axisConfig><axes size=\"dynamic\">"
                "<value0><axisId>0</axisId><axisName>x</axisName>"
                "<servoSlaveIds size=\"dynamic\"><value0>0</value0></servoSlaveIds>"
                "<maxVel>1</maxVel><maxAcc>2</maxAcc><maxJerk>3</maxJerk>"
                "<posPositiveLimit>10</posPositiveLimit><posNegativeLimit>-10</posNegativeLimit>"
                "<maxPosDiff>1</maxPosDiff><lead>0</lead></value0>"
                "</axes></axisConfig></cereal>",
                servoXml(0, 1), modelXml());
    assert(loadFails("bad-axis-lead"));

    // 回归：cereal 原生文件字段名写错必须报错，而不是静默变成空配置。
    {
        const auto dir = temp / "config" / "misspelled-field";
        // 把 <lead> 拼成 <leed>，其它字段完整。
        writeText(dir / "axis.xml",
                  "<?xml version=\"1.0\" encoding=\"UTF-8\"?><cereal><axisConfig>"
                  "<axes size=\"dynamic\">"
                  "<value0><axisId>0</axisId><axisName>x</axisName>"
                  "<servoSlaveIds size=\"dynamic\"><value0>0</value0></servoSlaveIds>"
                  "<maxVel>1</maxVel><maxAcc>2</maxAcc><maxJerk>3</maxJerk>"
                  "<posPositiveLimit>10</posPositiveLimit><posNegativeLimit>-10</posNegativeLimit>"
                  "<maxPosDiff>1</maxPosDiff><leed>5</leed></value0>"
                  "</axes></axisConfig></cereal>");
        writeText(dir / "servo.xml", servoXml(0, 1));
        writeText(dir / "model.xml", modelXml());
        assert(loadFails("misspelled-field"));
    }

    // 回归：cereal 原生文件即使语法合法，空 <axes> 也必须被拦截（不是零轴启动）。
    {
        const auto dir = temp / "config" / "empty-axes";
        writeText(dir / "axis.xml",
                  "<?xml version=\"1.0\" encoding=\"UTF-8\"?><cereal><axisConfig>"
                  "<axes size=\"dynamic\"></axes></axisConfig></cereal>");
        writeText(dir / "servo.xml", servoXml(0, 1));
        writeText(dir / "model.xml", modelXml());
        assert(loadFails("empty-axes"));
    }

    std::filesystem::current_path(original);
    std::filesystem::remove_all(temp);
    return 0;
}

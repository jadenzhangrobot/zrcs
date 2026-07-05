#include "ConfigSerializer.h"

#include <cereal/archives/xml.hpp>
#include "tinyxml2.h"

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <type_traits>

namespace zrcs::config {
namespace {

// 这里保留 tinyxml2 回退解析工具，是为了迁移期间还能读取旧的手写属性风格 XML。
// 新的业务配置文件应通过 cereal 写出；这些回退路径只用于让旧项目文件在数据确实
// 无效时才失败，而不是因为格式迁移立刻全部不可读。
void loadDocument(const std::filesystem::path& path, tinyxml2::XMLDocument& doc)
{
    const auto status = doc.LoadFile(path.string().c_str());
    if (status == tinyxml2::XML_ERROR_FILE_NOT_FOUND) {
        throw std::runtime_error("can't find " + path.string());
    }
    if (status != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error(path.string() + " has XML syntax error");
    }
    if (doc.RootElement() == nullptr) {
        throw std::runtime_error(path.string() + " has no root element");
    }
}

const char* requireString(tinyxml2::XMLElement* elem, const char* name)
{
    const char* value = elem ? elem->Attribute(name) : nullptr;
    if (value == nullptr) {
        throw std::runtime_error(std::string("missing XML attribute: ") + name);
    }
    return value;
}

template <typename T>
T queryNumber(tinyxml2::XMLElement* elem, const char* name)
{
    const char* value = requireString(elem, name);
    if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(std::stod(value));
    } else {
        return static_cast<T>(std::stoll(value, nullptr, 0));
    }
}

template <typename T>
T queryNumberDefault(tinyxml2::XMLElement* elem, const char* name, T defaultValue)
{
    const char* value = elem ? elem->Attribute(name) : nullptr;
    if (value == nullptr) {
        return defaultValue;
    }
    if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(std::stod(value));
    } else {
        return static_cast<T>(std::stoll(value, nullptr, 0));
    }
}

tinyxml2::XMLElement* requireChild(tinyxml2::XMLElement* parent, const char* name)
{
    auto* child = parent ? parent->FirstChildElement(name) : nullptr;
    if (child == nullptr) {
        throw std::runtime_error(std::string("missing XML element: ") + name);
    }
    return child;
}

PoseConfig parsePose(tinyxml2::XMLElement* elem)
{
    PoseConfig pose;
    if (elem == nullptr) {
        return pose;
    }
    pose.x = queryNumberDefault<double>(elem, "x", 0.0);
    pose.y = queryNumberDefault<double>(elem, "y", 0.0);
    pose.z = queryNumberDefault<double>(elem, "z", 0.0);
    pose.rx = queryNumberDefault<double>(elem, "rx", 0.0);
    pose.ry = queryNumberDefault<double>(elem, "ry", 0.0);
    pose.rz = queryNumberDefault<double>(elem, "rz", 0.0);
    return pose;
}

// 旧 model.xml 使用属性保存位姿字段。新的 cereal XML 会把相同标量保存成元素。
// 保留这组读写辅助函数，方便旧测试数据和临时项目文件迁移。
void writePose(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* parent,
               const char* name, const PoseConfig& pose)
{
    auto* elem = doc.NewElement(name);
    elem->SetAttribute("x", pose.x);
    elem->SetAttribute("y", pose.y);
    elem->SetAttribute("z", pose.z);
    elem->SetAttribute("rx", pose.rx);
    elem->SetAttribute("ry", pose.ry);
    elem->SetAttribute("rz", pose.rz);
    parent->InsertEndChild(elem);
}

void saveDocument(const std::filesystem::path& path, tinyxml2::XMLDocument& doc)
{
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    const auto status = doc.SaveFile(path.string().c_str());
    if (status != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("failed to write " + path.string());
    }
}

} // namespace

template <>
AxisConfigFile loadXml<AxisConfigFile>(const std::filesystem::path& path)
{
    try {
        // 优先路径：读取根对象名为 "axisConfig" 的 cereal 原生 XML。
        std::ifstream is(path);
        if (!is.is_open()) {
            throw std::runtime_error("can't find " + path.string());
        }
        cereal::XMLInputArchive archive(is);
        AxisConfigFile file;
        archive(cereal::make_nvp("axisConfig", file));
        return file;
    } catch (const cereal::Exception&) {
    }

    // 迁移回退路径：旧 axis.xml 形如
    // <axisConfig><axis id="0" ...><servos><servo slaveId="0"/></servos>.
    tinyxml2::XMLDocument doc;
    loadDocument(path, doc);
    auto* root = doc.RootElement();
    AxisConfigFile file;

    for (auto* axisElem = root->FirstChildElement("axis"); axisElem;
         axisElem = axisElem->NextSiblingElement("axis")) {
        AxisConfigData axis;
        axis.axisId = queryNumber<uint32_t>(axisElem, "id");
        axis.axisName = requireString(axisElem, "name");
        axis.maxVel = queryNumber<double>(axisElem, "maxVel");
        axis.maxAcc = queryNumber<double>(axisElem, "maxAcc");
        axis.maxJerk = queryNumber<double>(axisElem, "maxJerk");
        axis.posPositiveLimit = queryNumber<double>(axisElem, "maxPos");
        axis.posNegativeLimit = queryNumber<double>(axisElem, "minPos");
        axis.maxPosDiff = queryNumber<double>(axisElem, "maxPosDiff");
        axis.lead = queryNumber<double>(axisElem, "lead");

        auto* servosElem = requireChild(axisElem, "servos");
        for (auto* ref = servosElem->FirstChildElement("servo"); ref;
             ref = ref->NextSiblingElement("servo")) {
            axis.servoSlaveIds.push_back(queryNumber<uint32_t>(ref, "slaveId"));
        }
        file.axes.push_back(axis);
    }
    return file;
}

template <>
ServoConfigFile loadXml<ServoConfigFile>(const std::filesystem::path& path)
{
    try {
        // 优先路径：读取根对象名为 "servoConfig" 的 cereal 原生 XML。
        std::ifstream is(path);
        if (!is.is_open()) {
            throw std::runtime_error("can't find " + path.string());
        }
        cereal::XMLInputArchive archive(is);
        ServoConfigFile file;
        archive(cereal::make_nvp("servoConfig", file));
        return file;
    } catch (const cereal::Exception& e) {
        tinyxml2::XMLDocument probe;
        loadDocument(path, probe);
        if (std::string(probe.RootElement()->Name()) == "cereal") {
            throw std::runtime_error("invalid cereal servo.xml " + path.string() +
                                     ": " + e.what());
        }
    }

    // 迁移回退路径：旧 servo.xml 每个驱动器是一个扁平的 <servo .../> 元素。
    // 旧字段名 "posFactor" 会映射到 encoderCountPerUnit。direction 不做兼容默认值，
    // 旧格式如果仍在使用，也必须显式写 direction="1" 或 direction="-1"。
    tinyxml2::XMLDocument doc;
    loadDocument(path, doc);
    auto* root = doc.RootElement();
    ServoConfigFile file;

    for (auto* servoElem = root->FirstChildElement("servo"); servoElem;
         servoElem = servoElem->NextSiblingElement("servo")) {
        ServoConfigData servo;
        servo.slaveId = queryNumber<uint32_t>(servoElem, "slaveId");
        servo.mode = requireString(servoElem, "mode");
        servo.encoderCountPerUnit = queryNumber<uint64_t>(servoElem, "posFactor");
        servo.direction = queryNumber<int>(servoElem, "direction");
        servo.homePos = queryNumberDefault<double>(servoElem, "homePos", 0.0);
        servo.posOffset = queryNumberDefault<double>(servoElem, "posOffset", 0.0);
        servo.velFactor = queryNumberDefault<double>(servoElem, "velFactor", 1.0);
        file.servos.push_back(servo);
    }
    return file;
}

template <>
ModelConfigFile loadXml<ModelConfigFile>(const std::filesystem::path& path)
{
    try {
        // 优先路径：读取根对象名为 "modelConfig" 的 cereal 原生 XML。
        std::ifstream is(path);
        if (!is.is_open()) {
            throw std::runtime_error("can't find " + path.string());
        }
        cereal::XMLInputArchive archive(is);
        ModelConfigFile file;
        archive(cereal::make_nvp("modelConfig", file));
        return file;
    } catch (const cereal::Exception&) {
    }

    // 迁移回退路径：旧 model.xml 使用较多属性式 model/joint 节点。
    // 解析后仍统一转换成 ModelConfigData，运行时不需要关心来源格式。
    tinyxml2::XMLDocument doc;
    loadDocument(path, doc);
    auto* root = doc.RootElement();
    ModelConfigFile file;

    for (auto* modelElem = root->FirstChildElement("model"); modelElem;
         modelElem = modelElem->NextSiblingElement("model")) {
        ModelConfigData model;
        model.name = requireString(modelElem, "name");
        model.type = requireString(modelElem, "type");
        model.dof = queryNumber<int>(modelElem, "dof");
        model.baseFrame = parsePose(modelElem->FirstChildElement("baseFrame"));
        model.toolFrame = parsePose(modelElem->FirstChildElement("toolFrame"));

        if (auto* geo = modelElem->FirstChildElement("geometry")) {
            model.basePlatformRadius = queryNumberDefault<double>(geo, "basePlatformRadius", 0.0);
            model.mobilePlatformRadius = queryNumberDefault<double>(geo, "mobilePlatformRadius", 0.0);
            model.upperArmLength = queryNumberDefault<double>(geo, "upperArmLength", 0.0);
            model.lowerArmLength = queryNumberDefault<double>(geo, "lowerArmLength", 0.0);
        }

        auto* jointsElem = requireChild(modelElem, "joints");
        for (auto* jointElem = jointsElem->FirstChildElement("joint"); jointElem;
             jointElem = jointElem->NextSiblingElement("joint")) {
            ModelJointConfigData joint;
            joint.axisId = queryNumber<int>(jointElem, "axisId");
            joint.type = requireString(jointElem, "type");
            joint.offset = queryNumberDefault<double>(jointElem, "offset", 0.0);
            joint.dh_a = queryNumberDefault<double>(jointElem, "dh_a", 0.0);
            joint.dh_alpha = queryNumberDefault<double>(jointElem, "dh_alpha", 0.0);
            joint.dh_d = queryNumberDefault<double>(jointElem, "dh_d", 0.0);
            joint.dh_theta = queryNumberDefault<double>(jointElem, "dh_theta", 0.0);
            const char* axisAttr = jointElem->Attribute("axis");
            joint.axis = axisAttr && axisAttr[0] ? axisAttr[0] : 'X';
            model.joints.push_back(joint);
        }
        file.models.push_back(model);
    }
    return file;
}

template <>
void saveXml<AxisConfigFile>(const std::filesystem::path& path, const AxisConfigFile& value)
{
    // axis.xml 直接使用 cereal 输出。固定根 NVP 可以保证不同调用点写出的
    // 文件根节点一致。
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream os(path);
    if (!os.is_open()) {
        throw std::runtime_error("failed to write " + path.string());
    }
    cereal::XMLOutputArchive archive(os);
    archive(cereal::make_nvp("axisConfig", value));
}

template <>
void saveXml<ServoConfigFile>(const std::filesystem::path& path, const ServoConfigFile& value)
{
    // servo.xml 当前仍由 cereal 写出。如果后续 XML 节点名需要成为人工编辑约定，
    // 只需要在这个特化里把 servo.xml 切到自定义 writer，不必影响 axis/model。
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream os(path);
    if (!os.is_open()) {
        throw std::runtime_error("failed to write " + path.string());
    }
    cereal::XMLOutputArchive archive(os);
    archive(cereal::make_nvp("servoConfig", value));
}

template <>
void saveXml<ModelConfigFile>(const std::filesystem::path& path, const ModelConfigFile& value)
{
    // model.xml 只保存位姿标量，加载后再由 ModelConfig 转成 Eigen 矩阵。
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream os(path);
    if (!os.is_open()) {
        throw std::runtime_error("failed to write " + path.string());
    }
    cereal::XMLOutputArchive archive(os);
    archive(cereal::make_nvp("modelConfig", value));
}

} // namespace zrcs::config

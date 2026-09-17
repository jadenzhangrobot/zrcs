#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <cereal/archives/xml.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

namespace zrcs::config {

/**
 * @brief 文件类型 → XML 根节点名的映射。
 *
 * loadXml/saveXml 通过这个 trait 固定每个文件类型的根元素名，调用点不需要
 * 知道文件名级约定。新增文件类型时只需定义数据结构和本 trait 的特化
 * （参见 AxisConfig.h / ServoConfig.h / ModelConfig.h / EthercatConfig.h /
 * MujocoConfig.h）。
 */
template <typename T>
struct XmlRoot;

// 通过检测 T 是否有 XmlRoot<T>::name 来判断该类型是否可序列化到 XML。
template <typename T, typename = void>
struct is_xml_file : std::false_type {};

template <typename T>
struct is_xml_file<T, std::void_t<decltype(XmlRoot<T>::name)>> : std::true_type {};

/**
 * @brief 加载一个强类型 XML 配置对象。
 *
 * 这是本目录唯一的 XML 解析入口：全部业务配置文件（axis/servo/model，
 * 以及 ethercat/mujoco）都统一走 cereal。文件根元素必须与本类型 XmlRoot
 * 声明的名字一致，任何解析/数据错误都会带上根节点名和文件路径抛出，不会
 * 静默变成空对象。
 *
 * @throws std::runtime_error 文件缺失、XML 格式错误或字段缺失/数值非法时抛出。
 */
template <typename T>
T loadXml(const std::filesystem::path& path)
{
    static_assert(is_xml_file<T>::value,
                  "loadXml<T> requires an XmlRoot<T> specialization "
                  "(define it or include the config's header)");
    std::ifstream is(path);
    if (!is.is_open()) {
        throw std::runtime_error("can't find " + path.string());
    }
    try {
        cereal::XMLInputArchive archive(is);
        T file;
        archive(cereal::make_nvp(XmlRoot<T>::name(), file));
        return file;
    } catch (const std::exception& e) {
        // 统一包成带根节点名和文件路径的错误。cereal 抛出的 cereal::Exception
        // 以及 std::stoul/stod 的 invalid_argument/out_of_range 都从这里转出去。
        throw std::runtime_error(std::string("invalid XML \"") + XmlRoot<T>::name() +
                                 "\" in " + path.string() + ": " + e.what());
    }
}

/**
 * @brief 保存一个强类型 XML 配置对象。
 *
 * 固定使用该类型的 XmlRoot 根节点名，保证不同调用点写出的文件根节点一致。
 */
template <typename T>
void saveXml(const std::filesystem::path& path, const T& value)
{
    static_assert(is_xml_file<T>::value,
                  "saveXml<T> requires an XmlRoot<T> specialization "
                  "(define it or include the config's header)");
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream os(path);
    if (!os.is_open()) {
        throw std::runtime_error("failed to write " + path.string());
    }
    cereal::XMLOutputArchive archive(os);
    archive(cereal::make_nvp(XmlRoot<T>::name(), value));
}

} // namespace zrcs::config

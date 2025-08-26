
#ifndef NODE_FACTORY_H_
#define NODE_FACTORY_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "basenodeInterface.h"

namespace zrcsSystem {

// 前向声明
class Basenode;
class OneShotNode;
class PersistentNode;

/**
 * @brief 节点工厂，使用静态反射实现
 * @tparam BaseType 节点的基类类型
 */
template <typename BaseType>
class NodeFactory {
public:
    using Creator = std::shared_ptr<BaseType>;

    /**
     * @brief 获取工厂单例
     */
    static NodeFactory& getInstance() {
        static NodeFactory instance;
        return instance;
    }

    /**
     * @brief 注册节点类型
     * @param name 节点类型名
     * @param creator 创建者函数
     */
    void regist(const std::string& name, Creator creator) {
        registry_[name] =creator;
    }

    /**
     * @brief 创建节点实例
     * @param name 节点类型名
     * @return 节点实例的 unique_ptr，如果类型未注册则返回 nullptr
     */
    Creator getNodePtr(const std::string& name) {
        auto it = registry_.find(name);
        if (it != registry_.end()) {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief 检查节点类型是否存在
     * @param name 节点类型名
     * @return 如果存在则为 true，否则为 false
     */
    bool exist(const std::string& name) const {
        return registry_.find(name) != registry_.end();
    }

private:
    NodeFactory() = default;
    ~NodeFactory() = default;
    NodeFactory(const NodeFactory&) = delete;
    NodeFactory& operator=(const NodeFactory&) = delete;

    std::unordered_map<std::string, Creator> registry_;
};

/**
 * @brief 用于自动注册的辅助类
 * @tparam T 节点类型
 * @tparam BaseType 节点的基类类型
 */
template <typename T, typename BaseType>
class RegisterNode {
public:
    RegisterNode(const std::string& name) {
        NodeFactory<BaseType>::getInstance().regist(name,  std::make_shared<T>());
    }
};

} // namespace zrcsSystem

#define REGISTER_NODE_IMPL(className, baseType, counter) \
    static zrcsSystem::RegisterNode<className, zrcsSystem::baseType> \
    register_##className##_##counter(#className);

#define REGISTER_NODE(className, baseType) \
    REGISTER_NODE_IMPL(className, baseType, __COUNTER__)

#define REGISTERCMD(className) REGISTER_NODE(className, OneShotNode)
#define REGISTERNODE(className) REGISTER_NODE(className, PersistentNode)

#endif // NODE_FACTORY_H_
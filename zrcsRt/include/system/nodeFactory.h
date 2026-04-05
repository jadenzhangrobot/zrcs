
#ifndef NODE_FACTORY_H_
#define NODE_FACTORY_H_
#include <memory>
#include <unordered_map>
#include "system/base/basenodeInterface.h"

// 前向声明
class ModelRegistry;

namespace zrcsSystem {

// 前向声明
class Basenode;
class CmdNode;

/**
 * @brief 节点工厂，使用静态反射实现
 * @tparam BaseType 节点的基类类型
 */
class NodeFactory {
public:
    using Creator = std::shared_ptr<CmdNode>;
    using Output = std::shared_ptr<OutputNode>;
    using Input = std::shared_ptr<InputNode>;
    std::vector<Output> outPutNodes;
    std::vector<Input> inPutNodes;
    ZrcsHardware::Controller *control=nullptr;
    RTProcess *rtProcess=nullptr;
    ModelRegistry *modelRegistry=nullptr;  // 多模型注册表7+

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
    void regist(const std::string_view& name, Creator creator) {
        registry_[name] =creator;
    }
   
    void addOutputNode(Output node) {
        outPutNodes.push_back(node);
    }
    void addInputNode(Input node) {
        inPutNodes.push_back(node);
    }
    /**
     * @brief 创建节点实例
     * @param name 节点类型名
     * @return 节点实例的 unique_ptr，如果类型未注册则返回 nullptr
     */
    Creator getNodePtr(const std::string_view& name) {
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
    bool exist(const std::string_view& name) const {
        return registry_.find(name) != registry_.end();
    }



private:
    NodeFactory() = default;
    ~NodeFactory() = default;
    NodeFactory(const NodeFactory&) = delete;
    NodeFactory& operator=(const NodeFactory&) = delete;
    std::unordered_map<std::string_view, Creator> registry_;
};

/**
 * @brief 用于自动注册的辅助类
 * @tparam T 节点类型
 * @tparam BaseType 节点的基类类型
 */
template <typename T>
class RegisterNode {
public:
    RegisterNode(const std::string_view& name) 
    {
        auto node = std::make_shared<T>();
        zrcsSystem::NodeFactory::getInstance().regist(name,  node);
    }
};

template<typename T>
class registerAndAddOutputNode {
public:
    registerAndAddOutputNode() {

        auto node = std::make_shared<T>();
        zrcsSystem::NodeFactory::getInstance().addOutputNode(node);
    }
};
template<typename T>
class registerAndAddInputNode {
public:
    registerAndAddInputNode() {
        auto node = std::make_shared<T>();
        zrcsSystem::NodeFactory::getInstance().addInputNode(node);
    }
};

} // namespace zrcsSystem

#define REGISTERCMD(className)\
    static zrcsSystem::RegisterNode<className> register##className(#className);

#define REGISTEROUTPUT(className)\
    static zrcsSystem::registerAndAddOutputNode<className> register_##className;

#define REGISTERINPUT(className)\
    static zrcsSystem::registerAndAddInputNode<className> register_##className;
   

#endif // NODE_FACTORY_H_
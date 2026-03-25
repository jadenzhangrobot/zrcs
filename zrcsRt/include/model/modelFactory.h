/**
 * @file modelFactory.h
 * @brief 模型工厂 + 多模型注册表
 *
 * ModelFactory: 根据 XML 配置的 type 字段创建对应的 RobotModel 子类
 * ModelRegistry: 管理多个模型实例，支持按名称/索引查找
 */
#ifndef MODEL_FACTORY_H
#define MODEL_FACTORY_H

#include "robotModel.h"
#include "serialRobot.h"
#include "cartesianRobot.h"
#include "parallelRobot.h"
#include "modelConfig.h"
#include <memory>
#include <unordered_map>

/**
 * @brief 模型工厂
 * 根据 ModelParam 中的 type 字段创建对应的 RobotModel 实例
 */
class ModelFactory
{
public:
    static std::unique_ptr<RobotModel> create(const ModelParam& param)
    {
        if (param.type == "serial")
        {
            return std::make_unique<SerialRobot>(
                param.name, param.dof, param.joints,
                param.baseTf, param.toolTf);
        }
        else if (param.type == "delta")
        {
            return std::make_unique<DeltaRobot>(
                param.name, param.joints,
                param.basePlatformRadius,
                param.mobilePlatformRadius,
                param.upperArmLength,
                param.lowerArmLength,
                param.baseTf, param.toolTf);
        }
        else if (param.type == "cartesian")
        {
            return std::make_unique<CartesianRobot>(
                param.name, param.dof, param.joints,
                param.baseTf, param.toolTf);
        }

        return nullptr;
    }
};

/**
 * @brief 多模型注册表
 *
 * 管理多个 RobotModel 实例，支持:
 * - 按名称查找 (CmdNode 通过命令参数指定目标模型)
 * - 按索引查找 (默认第0个)
 *
 * 使用场景: 同一控制器下驱动多个不同类型的机器人
 * (如 6轴机械臂 + 3轴龙门架)
 */
class ModelRegistry
{
private:
    std::vector<std::unique_ptr<RobotModel>> models_;
    std::unordered_map<std::string, RobotModel*> nameMap_;

public:
    /**
     * @brief 从配置批量创建所有模型
     */
    void loadFromConfig(const ModelConfig& config)
    {
        for (const auto& param : config.modelParams)
        {
            auto model = ModelFactory::create(param);
            if (model)
            {
                nameMap_[model->getName()] = model.get();
                models_.push_back(std::move(model));
            }
        }
    }

    /**
     * @brief 按名称查找模型
     * @return 模型指针，未找到返回 nullptr
     */
    RobotModel* getModel(const std::string& name) const
    {
        auto it = nameMap_.find(name);
        if (it != nameMap_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief 按索引查找模型
     * @return 模型指针，越界返回 nullptr
     */
    RobotModel* getModel(int index) const
    {
        if (index >= 0 && index < static_cast<int>(models_.size()))
        {
            return models_[index].get();
        }
        return nullptr;
    }

    const std::vector<std::unique_ptr<RobotModel>>& getAllModels() const
    {
        return models_;
    }

    size_t size() const { return models_.size(); }
};

#endif // MODEL_FACTORY_H

/**
 * @file modelFactory.cpp
 * @brief 模型工厂 + 多模型注册表实现
 */
#include "model/modelFactory.h"
#include "model/serialRobot.h"
#include "model/parallelRobot.h"
#include "model/cartesianRobot.h"

std::unique_ptr<RobotModel> ModelFactory::create(const ModelParam& param)
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

void ModelRegistry::loadFromConfig(const ModelConfig& config)
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

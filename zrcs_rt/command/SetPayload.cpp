/*
 * @Description: 设置末端负载（ABB GripLoad）
 */
#include "command/SetPayload.h"

void SetPayload::init()
{
    auto* registry = modelRegistry_;
    if (!registry)
    {
        ERROR_PRINT("SetPayload: 模型注册表未初始化\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("SetPayload: 未找到模型(id=0)\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    double mass = command_->args[SetPayloadMass];
    model->setPayload(mass);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void SetPayload::run(void) {}
void SetPayload::exit(void) {}

REGISTERCMD(SetPayload);

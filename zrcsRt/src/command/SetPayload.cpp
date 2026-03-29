/*
 * @Description: 设置末端负载（ABB GripLoad）
 */
#include "command/SetPayload.h"

void SetPayload::init()
{
    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
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

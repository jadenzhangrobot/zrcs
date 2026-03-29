/*
 * @Description: 设置基坐标系/工件坐标系（ABB wobjdata / ZMC BASE）
 */
#include "command/SetBase.h"

void SetBase::init()
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

    Eigen::Matrix4d baseFrame = RobotModel::poseFromXYZRPY(
        command_->args[SetBaseX], command_->args[SetBaseY], command_->args[SetBaseZ],
        command_->args[SetBaseRX], command_->args[SetBaseRY], command_->args[SetBaseRZ]);

    model->setBaseFrame(baseFrame);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void SetBase::run(void) {}
void SetBase::exit(void) {}

REGISTERCMD(SetBase);

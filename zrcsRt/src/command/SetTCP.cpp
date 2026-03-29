/*
 * @Description: 设置工具坐标系（ABB tooldata）
 */
#include "command/SetTCP.h"

void SetTCP::init()
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

    Eigen::Matrix4d toolFrame = RobotModel::poseFromXYZRPY(
        command_->args[SetTCPX], command_->args[SetTCPY], command_->args[SetTCPZ],
        command_->args[SetTCPRX], command_->args[SetTCPRY], command_->args[SetTCPRZ]);

    model->setToolFrame(toolFrame);
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void SetTCP::run(void) {}
void SetTCP::exit(void) {}

REGISTERCMD(SetTCP);

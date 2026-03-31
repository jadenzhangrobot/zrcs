/*
 * @Description: 设置工具坐标系（ABB tooldata）
 */
#include "command/SetTCP.h"

void SetTCP::init()
{
    auto* registry = zrcsSystem::NodeFactory::getInstance().modelRegistry;
    if (!registry)
    {
        ERROR_PRINT("SetTCP: 模型注册表未初始化\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("SetTCP: 未找到模型(id=0)\n");
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

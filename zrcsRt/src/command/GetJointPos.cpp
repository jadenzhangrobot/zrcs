/*
 * @Description: 查询当前关节位置（ABB CJointT / 固高 GT_GetPos）
 */
#include "command/GetJointPos.h"

void GetJointPos::init()
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

    int dof = model->getDof();
    auto axisIds = model->getAxisIds();

    double* result = shm().jointPosResult();
    for (int i = 0; i < dof; i++)
    {
        result[i] = controller_->axiss[axisIds[i]]->actualPos();
    }

    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void GetJointPos::run(void) {}
void GetJointPos::exit(void) {}

REGISTERCMD(GetJointPos);

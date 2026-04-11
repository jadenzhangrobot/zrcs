/*
 * @Description: 查询当前关节位置（ABB CJointT / 固高 GT_GetPos）
 */
#include "command/GetJointPos.h"

void GetJointPos::init()
{
    auto* registry = modelRegistry_;
    if (!registry)
    {
        ERROR_PRINT("GetJointPos: 模型注册表未初始化\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("GetJointPos: 未找到模型(id=0)\n");
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

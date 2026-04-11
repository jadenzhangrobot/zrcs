/*
 * @Description: 查询当前笛卡尔位姿（ABB CRobT）
 */
#include "command/GetFK.h"

void GetFK::init()
{
    auto* registry = modelRegistry_;
    if (!registry)
    {
        ERROR_PRINT("GetFK: 模型注册表未初始化\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("GetFK: 未找到模型(id=0)\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    int dof = model->getDof();
    auto axisIds = model->getAxisIds();

    Eigen::VectorXd jointPos(dof);
    for (int i = 0; i < dof; i++)
    {
        jointPos(i) = controller_->axiss[axisIds[i]]->actualPos();
    }

    Eigen::Matrix4d toolPose;
    if (!model->forwardKinematics(jointPos, toolPose))
    {
        ERROR_PRINT("GetFK: FK 求解失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    zrcs::FkResultData fk{};
    fk.pose[0] = toolPose(0, 3);  // X
    fk.pose[1] = toolPose(1, 3);  // Y
    fk.pose[2] = toolPose(2, 3);  // Z

    // extract RPY from rotation matrix
    Eigen::Matrix3d R = toolPose.block<3,3>(0,0);
    fk.pose[4] = std::atan2(-R(2,0), std::sqrt(R(0,0)*R(0,0) + R(1,0)*R(1,0)));  // RY
    double cy = std::cos(fk.pose[4]);
    if (std::abs(cy) > 1e-6)
    {
        fk.pose[3] = std::atan2(R(2,1)/cy, R(2,2)/cy);  // RX
        fk.pose[5] = std::atan2(R(1,0)/cy, R(0,0)/cy);  // RZ
    }
    else
    {
        fk.pose[3] = std::atan2(R(0,1), R(1,1));
        fk.pose[5] = 0;
    }
    zrcs::lfl_write(shm()->fkResult, fk);

    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void GetFK::run(void) {}
void GetFK::exit(void) {}

REGISTERCMD(GetFK);

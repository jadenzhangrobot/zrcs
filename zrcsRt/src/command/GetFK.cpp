/*
 * @Description: 查询当前笛卡尔位姿（ABB CRobT）
 */
#include "command/GetFK.h"

void GetFK::init()
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

    Eigen::VectorXd jointPos(dof);
    for (int i = 0; i < dof; i++)
    {
        jointPos(i) = controller_->axiss[axisIds[i]]->actualPos();
    }

    Eigen::Matrix4d toolPose;
    if (!model->forwardKinematics(jointPos, toolPose))
    {
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }

    double* result = shm().fkResult();
    result[0] = toolPose(0, 3);  // X
    result[1] = toolPose(1, 3);  // Y
    result[2] = toolPose(2, 3);  // Z

    // extract RPY from rotation matrix
    Eigen::Matrix3d R = toolPose.block<3,3>(0,0);
    result[4] = std::atan2(-R(2,0), std::sqrt(R(0,0)*R(0,0) + R(1,0)*R(1,0)));  // RY
    double cy = std::cos(result[4]);
    if (std::abs(cy) > 1e-6)
    {
        result[3] = std::atan2(R(2,1)/cy, R(2,2)/cy);  // RX
        result[5] = std::atan2(R(1,0)/cy, R(0,0)/cy);  // RZ
    }
    else
    {
        result[3] = std::atan2(R(0,1), R(1,1));
        result[5] = 0;
    }

    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void GetFK::run(void) {}
void GetFK::exit(void) {}

REGISTERCMD(GetFK);

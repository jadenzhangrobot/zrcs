/*
 * @Description: 弧长参数化笛卡尔路径运动
 *
 * 数据流：NRT MotionPreprocessor → SHM pathQueue → RT PathMove
 *   1. initTrajectory(): 批量排空 pathQueue，计算累计弧长，初始化 1D Ruckig
 *   2. run(): 每周期 Ruckig 推进 s(t) → 查表插值 → IK → 关节指令
 */
#include "command/PathMove.h"
#include "shared_memory/ShmLayout.h"

PathMove::PathMove() : dof_(0), pathLen_(0), totalArc_(0)
{
    std::strcpy(nodeName_, "PathMove");
}

Result PathMove::updateTrajectory() { return otg_->update(*input_, *output_); }
void   PathMove::applyOutput() {}  // run() 中手动处理
void   PathMove::passOutputToInput() { output_->pass_to_input(*input_); }
void   PathMove::applyDeltaTime(double dt) { otg_->delta_time = dt; }

// ─────────────────────────────────────────────────────────────────────────────
// initTrajectory — 从 SHM pathQueue 批量排空路点，构建弧长表，初始化 Ruckig
// ─────────────────────────────────────────────────────────────────────────────

bool PathMove::initTrajectory()
{
    // 获取机器人模型
    auto* registry = modelRegistry_;
    if (!registry)
    {
        ERROR_PRINT("PathMove: 模型注册表未初始化\n");
        return false;
    }
    RobotModel* model = registry->getModel(0);
    if (!model)
    {
        ERROR_PRINT("PathMove: 未找到模型(id=0)\n");
        return false;
    }

    dof_ = model->getDof();
    axisIds_ = model->getAxisIds();

    // 从 SHM pathQueue 批量排空
    auto& queue = shm()->pathQueue;
    zrcs::ShmSPSCConsumer<zrcs::PathPoint, zrcs::kPathBufCap> consumer(queue);

    pathLen_ = 0;
    zrcs::PathPoint pt{};
    while (pathLen_ < kMaxPts && consumer.pop(pt))
    {
        auto& e = pathBuf_[pathLen_];
        e.x  = pt.x;   e.y  = pt.y;   e.z  = pt.z;
        e.rx = pt.rx;   e.ry = pt.ry;  e.rz = pt.rz;
        e.maxVel = pt.maxVel;
        e.s = 0;
        ++pathLen_;
    }

    if (pathLen_ < 2)
    {
        ERROR_PRINT("PathMove: 路径点不足 (got %zu)\n", pathLen_);
        return false;
    }

    // 计算累计弧长
    pathBuf_[0].s = 0;
    for (size_t i = 1; i < pathLen_; ++i)
    {
        double dx = pathBuf_[i].x - pathBuf_[i - 1].x;
        double dy = pathBuf_[i].y - pathBuf_[i - 1].y;
        double dz = pathBuf_[i].z - pathBuf_[i - 1].z;
        pathBuf_[i].s = pathBuf_[i - 1].s + std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    totalArc_ = pathBuf_[pathLen_ - 1].s;

    if (totalArc_ < 1e-6)
    {
        ERROR_PRINT("PathMove: 路径总弧长为零\n");
        return false;
    }

    // 读取 PathMoveConfig
    double maxVel   = shm()->pathMoveCfg.maxVel.load(std::memory_order_acquire);
    double maxAccel  = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
    double maxJerk   = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);

    // 速度倍率（命令参数）
    double velScale = command_->args[Vel];
    if (velScale > 0) maxVel *= velScale;

    // 初始化 1D Ruckig（弧长参数 s）
    otg_    = std::make_unique<Ruckig<DynamicDOFs>>(1, cycletime * 0.001);
    input_  = std::make_unique<InputParameter<DynamicDOFs>>(1);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(1);

    input_->current_position[0]     = 0;
    input_->current_velocity[0]     = 0;
    input_->current_acceleration[0] = 0;
    input_->target_position[0]      = totalArc_;
    input_->target_velocity[0]      = 0;
    input_->target_acceleration[0]  = 0;
    input_->max_velocity[0]         = std::min(maxVel, pathBuf_[0].maxVel);
    input_->max_acceleration[0]     = maxAccel;
    input_->max_jerk[0]             = maxJerk;

    // 清除使能标志
    shm()->pathMoveActive.store(false, std::memory_order_release);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// run — 每 RT 周期：Ruckig 推进 s(t) → 查表插值 → IK → 关节指令
// ─────────────────────────────────────────────────────────────────────────────

void PathMove::run()
{
    updateOverride();

    auto* registry = modelRegistry_;
    RobotModel* model = registry->getModel(0);

    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working || result == Result::Finished)
    {
        double s = output_->new_position[0];

        // 查表插值得到笛卡尔位姿
        double x, y, z, rx, ry, rz;
        lookupPath(s, x, y, z, rx, ry, rz);

        // 动态更新速度限制（Ruckig 在线重规划）
        double vLimit = lookupMaxVel(s);
        input_->max_velocity[0] = vLimit;

        // 当前关节位置
        Eigen::VectorXd currentJoint(dof_);
        for (int i = 0; i < dof_; i++)
        {
            currentJoint(i) = controller_->axiss[axisIds_[i]]->actualPos();
        }

        // 构建目标位姿
        Eigen::Matrix4d targetPose = RobotModel::poseFromXYZRPY(x, y, z, rx, ry, rz);

        // IK 求解
        Eigen::VectorXd targetJoint(dof_);
        if (!model->inverseKinematics(targetPose, currentJoint, targetJoint))
        {
            ERROR_PRINT("PathMove: IK 求解失败 (s=%.2f)\n", s);
            setCmdStatus(zrcsSystem::CmdStatus::FAILED);
            return;
        }

        // 写入关节指令
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[axisIds_[i]]->setAxisPositionCmd(targetJoint(i));
        }

        if (result == Result::Finished)
        {
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        }
        else
        {
            output_->pass_to_input(*input_);
        }
    }
    else
    {
        ERROR_PRINT("PathMove: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// lookupPath — 二分查找 + 线性插值：弧长 s → 笛卡尔位姿
// ─────────────────────────────────────────────────────────────────────────────

void PathMove::lookupPath(double s, double& x, double& y, double& z,
                          double& rx, double& ry, double& rz) const
{
    // 边界钳位
    if (s <= 0)
    {
        const auto& e = pathBuf_[0];
        x = e.x; y = e.y; z = e.z;
        rx = e.rx; ry = e.ry; rz = e.rz;
        return;
    }
    if (s >= totalArc_)
    {
        const auto& e = pathBuf_[pathLen_ - 1];
        x = e.x; y = e.y; z = e.z;
        rx = e.rx; ry = e.ry; rz = e.rz;
        return;
    }

    // 二分查找 s 所在段 [idx-1, idx]
    auto it = std::upper_bound(
        pathBuf_, pathBuf_ + pathLen_, s,
        [](double val, const PathEntry& e) { return val < e.s; });
    size_t idx = static_cast<size_t>(it - pathBuf_);
    if (idx == 0) idx = 1;
    if (idx >= pathLen_) idx = pathLen_ - 1;

    const auto& p0 = pathBuf_[idx - 1];
    const auto& p1 = pathBuf_[idx];
    double ds = p1.s - p0.s;
    double t = (ds > 1e-9) ? (s - p0.s) / ds : 0;

    x  = p0.x  + t * (p1.x  - p0.x);
    y  = p0.y  + t * (p1.y  - p0.y);
    z  = p0.z  + t * (p1.z  - p0.z);
    rx = p0.rx + t * (p1.rx - p0.rx);
    ry = p0.ry + t * (p1.ry - p0.ry);
    rz = p0.rz + t * (p1.rz - p0.rz);
}

// ─────────────────────────────────────────────────────────────────────────────
// lookupMaxVel — 二分查找 + 线性插值：弧长 s → 速度限制
// ─────────────────────────────────────────────────────────────────────────────

double PathMove::lookupMaxVel(double s) const
{
    if (s <= 0) return pathBuf_[0].maxVel;
    if (s >= totalArc_) return pathBuf_[pathLen_ - 1].maxVel;

    auto it = std::upper_bound(
        pathBuf_, pathBuf_ + pathLen_, s,
        [](double val, const PathEntry& e) { return val < e.s; });
    size_t idx = static_cast<size_t>(it - pathBuf_);
    if (idx == 0) idx = 1;
    if (idx >= pathLen_) idx = pathLen_ - 1;

    const auto& p0 = pathBuf_[idx - 1];
    const auto& p1 = pathBuf_[idx];
    double ds = p1.s - p0.s;
    double t = (ds > 1e-9) ? (s - p0.s) / ds : 0;

    return p0.maxVel + t * (p1.maxVel - p0.maxVel);
}

CMD_REGISTER(PathMove);

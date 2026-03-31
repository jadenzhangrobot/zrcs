/*
 * @Description: 缓冲区连续插补（固高 GT_CrdData / 正运动连续插补）
 *               预装多段轨迹到缓冲区，不停顿连续执行
 */
#include "command/BufMove.h"

void BufMove::init()
{
    int type = static_cast<int>(command_->args[BufMoveType]);

    // Each BufMove command adds one segment.
    // Type -1 signals "start execution" of previously buffered segments.
    if (type < 0)
    {
        // execute buffered segments — start from segment 0
        if (segments_.empty())
        {
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
            return;
        }
        segIdx_ = 0;
        setupSegment(segIdx_);
        return;
    }

    // buffer a new segment
    Segment seg;
    seg.type = type;
    seg.params[0] = command_->args[BufMoveP1];
    seg.params[1] = command_->args[BufMoveP2];
    seg.params[2] = command_->args[BufMoveP3];
    seg.params[3] = command_->args[BufMoveP4];
    seg.params[4] = command_->args[BufMoveP5];
    seg.vel = command_->args[BufMoveVel];
    segments_.push_back(seg);

    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void BufMove::setupSegment(int idx)
{
    if (idx >= static_cast<int>(segments_.size()))
    {
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        return;
    }

    // simplified: 3-axis line segment (X, Y, Z targets in params[0..2])
    dof_ = 3;
    if (dof_ > static_cast<int>(controller_->axiss.size()))
    {
        dof_ = static_cast<int>(controller_->axiss.size());
    }

    otg_ = std::make_unique<Ruckig<DynamicDOFs>>(dof_, cycletime * 0.001);
    input_ = std::make_unique<InputParameter<DynamicDOFs>>(dof_);
    output_ = std::make_unique<OutputParameter<DynamicDOFs>>(dof_);

    double override = shm().overrideRatio().load(std::memory_order_acquire);
    double velScale = segments_[idx].vel;
    if (velScale <= 0) velScale = 1.0;

    for (int i = 0; i < dof_; i++)
    {
        input_->current_position[i] = controller_->axiss[i]->actualPos();
        input_->current_velocity[i] = 0;
        input_->current_acceleration[i] = 0;
        input_->target_position[i] = segments_[idx].params[i];
        input_->target_velocity[i] = 0;
        input_->target_acceleration[i] = 0;
        input_->max_velocity[i] = controller_->axiss[i]->getMaxVelocity() * override * velScale;
        input_->max_acceleration[i] = controller_->axiss[i]->getMaxAcceleration();
        input_->max_jerk[i] = controller_->axiss[i]->getMaxJerk();
    }
}

void BufMove::run(void)
{
    if (!otg_)
    {
        ERROR_PRINT("BufMove: 段数据为空\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
        return;
    }
    auto result = otg_->update(*input_, *output_);
    if (result == Result::Working)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[i]->setAxisPositionCmd(output_->new_position[i]);
        }
        output_->pass_to_input(*input_);
    }
    else if (result == Result::Finished)
    {
        for (int i = 0; i < dof_; i++)
        {
            controller_->axiss[i]->setAxisPositionCmd(output_->new_position[i]);
        }

        segIdx_++;
        if (segIdx_ < static_cast<int>(segments_.size()))
        {
            setupSegment(segIdx_);
        }
        else
        {
            setCmdStatus(zrcsSystem::CmdStatus::EXIT);
        }
    }
    else
    {
        ERROR_PRINT("BufMove: 轨迹规划失败\n");
        setCmdStatus(zrcsSystem::CmdStatus::FAILED);
    }
}

void BufMove::exit(void) {}

REGISTERCMD(BufMove);

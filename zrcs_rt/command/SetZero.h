/**
 * @file    SetZero.h
 * @brief   设置当前位置为零点命令节点
 *
 * 将指定轴（或全部轴）的当前位置设置为零点。
 * 通过 setZeroOffset() 记录偏移量，同时将命令位置清零，
 * 后续的目标位置均相对于新的零点。
 */

#pragma once

#include <cstring>

#include "system/node/BaseNodeInterface.h"
#include "system/node/NodeFactory.h"

/**
 * @brief 设置当前位置为零点命令节点
 *
 * 继承自 CmdNode，实现零位设置逻辑。
 * 在 init() 中从命令参数读取轴 ID，
 * 在 run() 中调用对应轴的 setZeroOffset() 接口。
 */
class SetZero : public zrcsSystem::CmdNode
{
public:
    SetZero()
    {
        std::strcpy(nodeName_, "SetZero");
    }

    /** @brief 从命令参数读取目标轴 ID */
    void init() override;

    /** @brief 执行设置零位操作 */
    void run() override;

    /** @brief 退出清理（无操作） */
    void exit() override;

private:
    int axisId_;   ///< 目标轴 ID，等于轴总数时表示全部轴
};

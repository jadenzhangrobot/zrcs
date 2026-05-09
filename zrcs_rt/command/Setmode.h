/**
 * @file    Setmode.h
 * @brief   设置伺服控制模式命令节点
 *
 * 将指定轴（或全部轴）切换到目标 CiA 402 伺服控制模式（如 CSP、CSV、CST 等）。
 */

#pragma once

#include <cstring>

#include "system/node/BaseNodeInterface.h"
#include "system/node/NodeFactory.h"

/**
 * @brief 设置伺服控制模式命令节点
 *
 * 继承自 CmdNode，实现控制模式切换逻辑。
 * 在 init() 中从命令参数读取轴 ID 和目标模式，
 * 在 run() 中调用对应轴的 setModeOfOperation() 接口。
 */
class Setmode : public zrcsSystem::CmdNode
{
public:
    Setmode()
    {
        std::strcpy(nodeName_, "Setmode");
    }

    /** @brief 从命令参数读取目标轴 ID 和控制模式 */
    void init() override;

    /** @brief 执行模式切换 */
    void run() override;

    /** @brief 退出清理（无操作） */
    void exit() override;

private:
    int axisId_;   ///< 目标轴 ID，等于轴总数时表示全部轴
    int mode_;      ///< 目标 CiA 402 控制模式枚举值
};

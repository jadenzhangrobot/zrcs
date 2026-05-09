/**
 * @file    Reset.h
 * @brief   复位错误命令节点
 * @author  zhangyongjing
 * @date    2023-03-28
 *
 * 对单个轴或全部轴执行错误复位操作。复位成功后会将命令位置同步到
 * 当前实际位置，避免复位后发生跳变。
 */

#pragma once

#include <cstring>

#include "system/node/BaseNodeInterface.h"
#include "system/node/NodeFactory.h"

/**
 * @brief 复位错误命令节点
 *
 * 继承自 CmdNode，实现命令式错误复位逻辑。
 * 在 init() 中从命令参数读取轴 ID，
 * 在 run() 中调用对应轴的 resetError() 接口。
 */
class Reset : public zrcsSystem::CmdNode
{
public:
    Reset()
    {
        std::strcpy(nodeName_, "Reset");
    }

    /** @brief 从命令参数读取目标轴 ID */
    void init() override;

    /** @brief 执行错误复位操作 */
    void run() override;

    /** @brief 退出清理（无操作） */
    void exit() override;

private:
    int axisId_;   ///< 目标轴 ID，等于轴总数时表示全部轴
};

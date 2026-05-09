/**
 * @file    Enable.h
 * @brief   电机使能命令节点
 * @author  zhangyongjing
 * @date    2023-03-28
 *
 * 对单个轴或全部轴执行使能操作。如果传入的轴 ID 等于轴总数，
 * 则对所有轴依次使能；否则只使能指定轴。
 *
 * 使能前会先设置伺服控制模式、同步当前位置，确保上电后零速启动。
 */

#pragma once

#include <cstring>

#include "system/node/BaseNodeInterface.h"
#include "system/node/NodeFactory.h"

/**
 * @brief 电机使能命令节点
 *
 * 继承自 CmdNode，实现命令式使能逻辑。
 * 在 init() 中从命令参数读取轴 ID，
 * 在 run() 中调用对应轴的 powerOn() 接口。
 */
class Enable : public zrcsSystem::CmdNode
{
public:
    Enable()
    {
        std::strcpy(nodeName_, "Enable");
    }

    /** @brief 从命令参数读取目标轴 ID */
    void init() override;

    /** @brief 执行使能操作 */
    void run() override;

    /** @brief 退出清理（无操作） */
    void exit() override;

private:
    int axisId_;   ///< 目标轴 ID，等于轴总数时表示全部轴
};


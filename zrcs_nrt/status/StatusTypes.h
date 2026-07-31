#pragma once
/**
 * @file StatusTypes.h
 * @brief NRT 状态类型。
 *
 * BtStatus 供 BehaviorTreeRunner / BehaviorTreeService 和 StatusPublisher 使用。
 * 单位约定：直线轴 m / m·s⁻¹，旋转轴 rad / rad·s⁻¹（G21 mm 已在 NcParser 换算）。
 */

#include <string>

namespace zrcs_nrt {

/**
 * @brief 行为树运行状态，供 GUI 显示当前执行到哪个节点。
 */
struct BtStatus {
    std::string treeState;    ///< IDLE / LOADED / RUNNING / SUCCESS / FAILURE / HALTED
    std::string currentNode;  ///< 当前正在 tick 的节点名
    std::string message;      ///< 附加信息，失败时为错误原因
};

} // namespace zrcs_nrt

/**
 * @file AxisConfig.h
 * @brief 轴参数配置结构体 — 从 axis.xml 解析轴的运动学和伺服参数
 *
 * AxisPara 定义了单个轴的所有配置属性（ID、名称、控制模式、
 * 编码器分辨率、速度/加速度/加加速度限制、软限位和跟随误差阈值）。
 * AxisConfig 类负责从 XML 文件加载并填充 AxisPara 向量。
 *
 * @author zhangyongjing
 */
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "config/Parameter.h"
#include "Global.h"
#include "xml/XmlParsing.h"

namespace ZrcsHardware {

/**
 * @brief 单轴配置参数
 *
 * 每个运动轴对应一个 AxisPara 实例，存储轴的全部配置属性。
 * 这些参数在 axis.xml 中定义，由 AxisConfig 解析加载。
 */
typedef struct {
    uint32_t axisId;              ///< 轴 ID（全局唯一）
    uint32_t slaveId;             ///< 对应的从站 ID
    std::string axisName;         ///< 轴名称（如 "X", "Y", "Z"）
    MC_SERVO_CONTROL_MODE mode;   ///< 伺服控制模式（位置/速度/扭矩）
    uint64_t encoderCountPerUnit; ///< 编码器分辨率（counts/用户单位）
    double maxVel;                ///< 最大速度（用户单位/s）
    double maxAcc;                ///< 最大加速度（用户单位/s^2）
    double maxJerk;               ///< 最大加加速度（用户单位/s^3）
    double posPositiveLimit;      ///< 正向软限位（用户单位）
    double posNegativeLimit;      ///< 负向软限位（用户单位）
    double maxPosDiff;            ///< 最大跟随误差阈值（用户单位）
    double frequency;             ///< 控制频率（Hz）
} AxisPara;

/**
 * @brief 轴配置解析器
 *
 * 继承自 XmlParsing，从 axis.xml 文件加载所有轴的配置参数。
 * 解析后的数据存储在 axisParas 成员向量中。
 */
class AxisConfig : private XmlParsing {
public:
    std::vector<AxisPara> axisParas;  ///< 所有轴的配置参数列表

    /**
     * @brief 构造函数 — 从 XML 文件加载轴配置
     * @param xmlFileName axis.xml 的文件路径
     * @throws std::runtime_error 解析失败时抛出
     */
    AxisConfig(const std::string& xmlFileName);
};

} // namespace ZrcsHardware

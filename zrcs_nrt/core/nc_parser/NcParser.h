#pragma once
/**
 * @file NcParser.h
 * @brief NC/G-code → 路径点序列。
 *
 * 词法/句法：3rdParty/gpr（parse_gcode）
 * 语义：本文件将 G0/G1/G2/G3 解释为 waypoints（m，与控制器 SI 约定一致）
 *
 * 支持：
 *   - G20/G21 英制/公制（程序数值按标准语义：G21=mm，G20=inch；输出统一为 m）
 *   - G90/G91 绝对/相对
 *   - G0 / G1 直线
 *   - G2 / G3 圆弧（IJK 圆心偏置或 R 半径），按弦高采样为折线点
 *   - F 进给（模态；G21 为 mm/s，G20 为 inch/s；输出 m/s）
 *   - 括号注释、分号注释、N 行号、斜杠删除块、M/T 等（忽略）
 *
 * 不支持：刀补、坐标系变换、宏、子程序等完整 CNC 语义。
 *
 * 单位约定（控制器内部）：
 *   - 位置 / 长度 / 弦高：m
 *   - 进给：m/s
 *   - 姿态（下游 PathMove）：rad
 *
 * 注意：本项目 F 按「程序长度单位 / 秒」解析（G21→mm/s），
 * 不是部分机床默认的 mm/min。若 CAM 输出 mm/min，需在上游转换或扩展解析器。
 */

#include "algorithm/path_planning/TrajectoryTypes.h"

#include <string>
#include <vector>

class NcParser {
public:
    struct Config {
        /// 圆弧采样弦高误差 (m)，越小点越密。默认 0.2 mm。
        double arcChordTol = 0.0002;
        /// 圆弧最少采样分段数（不含终点）
        int arcMinSegments = 4;
    };

    struct Result {
        std::vector<Point3D> waypoints;
        /// 与 waypoints 相邻点对应的段进给 (m/s)；长度应为 waypoints.size()-1。
        /// <=0 表示该段未指定有效 F，规划时回退到全局 maxVel。
        /// 圆弧离散产生的多个子段共享该运动块的 F。
        std::vector<double> segmentFeedrates;
        std::string error; ///< 非空表示失败
        int lineCount = 0;
        int moveCount = 0;
    };

    NcParser() = default;
    explicit NcParser(Config cfg) : cfg_(cfg) {}

    /// 从文件路径解析。
    Result parseFile(const std::string& path) const;

    /// 从完整程序文本解析（底层走 gpr::parse_gcode）。
    Result parseText(const std::string& text) const;

private:
    Config cfg_;
};

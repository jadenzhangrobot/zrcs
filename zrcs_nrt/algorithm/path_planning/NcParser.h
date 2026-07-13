#pragma once
/**
 * @file NcParser.h
 * @brief NC/G-code → 路径点序列（纯算法）。
 *
 * 词法/句法：3rdParty/gpr（parse_gcode）
 * 语义：本文件将 G0/G1/G2/G3 解释为 waypoints（mm）
 *
 * 支持：
 *   - G20/G21 英制/公制（输出始终为 mm）
 *   - G90/G91 绝对/相对
 *   - G0 / G1 直线
 *   - G2 / G3 圆弧（IJK 圆心偏置或 R 半径），按弦高采样为折线点
 *   - 括号注释、分号注释、N 行号、斜杠删除块、M/T 等（忽略）
 *
 * 不支持：刀补、坐标系变换、宏、子程序等完整 CNC 语义。
 */

#include "algorithm/path_planning/TrajectoryTypes.h"

#include <string>
#include <vector>

class NcParser {
public:
    struct Config {
        /// 圆弧采样弦高误差 (mm)，越小点越密
        double arcChordTol = 0.2;
        /// 圆弧最少采样分段数（不含终点）
        int arcMinSegments = 4;
    };

    struct Result {
        std::vector<Point3D> waypoints;
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

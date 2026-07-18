#include "core/nc_parser/NcParser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

// 3rdParty/gpr — 词法/句法（include 目录由 gpr::gpr 目标 PUBLIC 传播）
#include "parser.h"

namespace {

constexpr double kPi = 3.14159265358979323846;
// 程序侧：G21 数值为 mm，G20 为 inch；控制器内部统一为 m。
constexpr double kMmToM = 0.001;
constexpr double kInchToM = 0.0254;
constexpr double kEps = 1e-12;

double addrAsDouble(const gpr::addr& a)
{
    if (a.tp() == gpr::ADDRESS_TYPE_DOUBLE) {
        return a.double_value();
    }
    return static_cast<double>(a.int_value());
}

// 从 gpr block 提取词地址表（注释/百分号忽略）；G/M 取整数值。
void collectWords(const gpr::block& b, std::unordered_map<char, double>& words)
{
    words.clear();
    for (const auto& c : b) {
        if (c.tp() != gpr::CHUNK_TYPE_WORD_ADDRESS) {
            continue;
        }
        const char letter = static_cast<char>(
            std::toupper(static_cast<unsigned char>(c.get_word())));
        words[letter] = addrAsDouble(c.get_address());
    }
}

// 追加路径点；若实际新增点且提供 feedOut，则为上一→本点段写入 feedrateMs（m/s）。
bool appendIfMoved(std::vector<Point3D>& waypoints,
                   const Point3D& p,
                   std::vector<double>* feedOut = nullptr,
                   double feedrateMs = 0.0)
{
    if (waypoints.empty()) {
        waypoints.push_back(p);
        return true;
    }
    if (pointDistance(waypoints.back(), p) > kEps) {
        waypoints.push_back(p);
        if (feedOut) {
            feedOut->push_back(feedrateMs);
        }
        return true;
    }
    return false;
}

std::vector<Point3D> sampleArc(const Point3D& start,
                               const Point3D& end,
                               double iOff,
                               double jOff,
                               double kOff,
                               bool clockwise,
                               double chordTol,
                               int minSegments)
{
    const Point3D center{start.x + iOff, start.y + jOff, start.z + kOff};
    const Point3D v0 = pointSub(start, center);
    const Point3D v1 = pointSub(end, center);
    const double r0 = pointLength(v0);
    if (r0 < kEps) {
        return {end};
    }

    const double a0 = std::atan2(v0.y, v0.x);
    const double a1 = std::atan2(v1.y, v1.x);
    double sweep = a1 - a0;
    if (clockwise) {
        if (sweep >= 0.0) {
            sweep -= 2.0 * kPi;
        }
    } else {
        if (sweep <= 0.0) {
            sweep += 2.0 * kPi;
        }
    }
    if (pointDistance(start, end) < kEps && std::abs(sweep) < kEps) {
        sweep = clockwise ? -2.0 * kPi : 2.0 * kPi;
    }

    const double absSweep = std::abs(sweep);
    const double r = r0;
    int n = minSegments;
    if (r > kEps && chordTol > 0.0) {
        const double dTheta = std::sqrt(std::max(8.0 * chordTol / r, 1e-12));
        n = std::max(n, static_cast<int>(std::ceil(absSweep / dTheta)));
    }
    n = std::max(n, 1);

    std::vector<Point3D> pts;
    pts.reserve(static_cast<size_t>(n));
    for (int s = 1; s <= n; ++s) {
        const double t = static_cast<double>(s) / static_cast<double>(n);
        const double ang = a0 + sweep * t;
        Point3D p;
        p.x = center.x + r * std::cos(ang);
        p.y = center.y + r * std::sin(ang);
        p.z = start.z + (end.z - start.z) * t;
        pts.push_back(p);
    }
    pts.back() = end;
    return pts;
}

bool radiusToIjk(const Point3D& start,
                 const Point3D& end,
                 double rSigned,
                 bool clockwise,
                 double& iOff,
                 double& jOff,
                 std::string* error)
{
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    const double chord = std::sqrt(dx * dx + dy * dy);
    if (chord < kEps) {
        if (error) {
            *error = "G2/G3 with R requires distinct start and end in XY";
        }
        return false;
    }
    const double r = std::abs(rSigned);
    if (r * 2.0 + kEps < chord) {
        if (error) {
            *error = "G2/G3 R too small for chord length";
        }
        return false;
    }

    const double mx = (start.x + end.x) * 0.5;
    const double my = (start.y + end.y) * 0.5;
    const double h = std::sqrt(std::max(r * r - (chord * 0.5) * (chord * 0.5), 0.0));
    const double nx = -dy / chord;
    const double ny = dx / chord;

    const double c1x = mx + nx * h;
    const double c1y = my + ny * h;
    const double c2x = mx - nx * h;
    const double c2y = my - ny * h;

    auto sweepAbs = [&](double cx, double cy) {
        const double a0 = std::atan2(start.y - cy, start.x - cx);
        double a1 = std::atan2(end.y - cy, end.x - cx);
        double sw = a1 - a0;
        if (clockwise) {
            if (sw >= 0.0) {
                sw -= 2.0 * kPi;
            }
        } else {
            if (sw <= 0.0) {
                sw += 2.0 * kPi;
            }
        }
        return std::abs(sw);
    };

    const double s1 = sweepAbs(c1x, c1y);
    const double s2 = sweepAbs(c2x, c2y);
    const bool wantLong = rSigned < 0.0;
    double cx = c1x;
    double cy = c1y;
    if (wantLong) {
        if (s1 < s2) {
            cx = c2x;
            cy = c2y;
        }
    } else {
        if (s1 > s2) {
            cx = c2x;
            cy = c2y;
        }
    }

    iOff = cx - start.x;
    jOff = cy - start.y;
    return true;
}

} // namespace

NcParser::Result NcParser::parseFile(const std::string& path) const
{
    Result result;
    std::ifstream in(path);
    if (!in) {
        result.error = "Cannot open NC file: " + path;
        return result;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return parseText(ss.str());
}

NcParser::Result NcParser::parseText(const std::string& text) const
{
    Result result;

    gpr::gcode_program program = gpr::parse_gcode(text);
    result.lineCount = program.num_blocks();

    Point3D pos{};
    bool havePos = false;
    bool absolute = true; // G90
    bool metric = true;   // G21
    int motion = 1;       // 默认 G1 模态
    double modalFeedMs = 0.0; // 模态 F，单位 m/s；<=0 表示未指定

    for (int bi = 0; bi < program.num_blocks(); ++bi) {
        gpr::block b = program.get_block(static_cast<size_t>(bi));
        if (b.is_deleted()) {
            continue;
        }

        std::unordered_map<char, double> words;
        collectWords(b, words);
        if (words.empty()) {
            continue;
        }

        if (words.count('G')) {
            const int g = static_cast<int>(std::lround(words['G']));
            switch (g) {
            case 0:
            case 1:
            case 2:
            case 3:
                motion = g;
                break;
            case 20:
                metric = false;
                break;
            case 21:
                metric = true;
                break;
            case 90:
                absolute = true;
                break;
            case 91:
                absolute = false;
                break;
            default:
                break;
            }
        }

        // G21: program units mm → m; G20: program units inch → m.
        // F: program length-unit / s → m/s（本项目约定，非 mm/min）。
        auto scale = [metric](double v) {
            return metric ? (v * kMmToM) : (v * kInchToM);
        };

        if (words.count('F')) {
            const double fProg = words['F'];
            if (std::isfinite(fProg) && fProg > 0.0) {
                modalFeedMs = scale(fProg);
            }
        }

        const bool hasAxis = words.count('X') || words.count('Y') || words.count('Z') ||
                             words.count('I') || words.count('J') || words.count('K') ||
                             words.count('R');
        if (!hasAxis) {
            continue;
        }

        int thisMotion = motion;
        if (words.count('G')) {
            const int g = static_cast<int>(std::lround(words['G']));
            if (g == 0 || g == 1 || g == 2 || g == 3) {
                thisMotion = g;
            } else if (!words.count('X') && !words.count('Y') && !words.count('Z')) {
                continue;
            }
        }

        // G0 快速移动：进给记 0，规划侧回退到全局 maxVel。
        const double segFeed = (thisMotion == 0) ? 0.0 : modalFeedMs;

        Point3D target = pos;
        if (absolute) {
            if (words.count('X')) {
                target.x = scale(words['X']);
            }
            if (words.count('Y')) {
                target.y = scale(words['Y']);
            }
            if (words.count('Z')) {
                target.z = scale(words['Z']);
            }
        } else {
            if (words.count('X')) {
                target.x = pos.x + scale(words['X']);
            }
            if (words.count('Y')) {
                target.y = pos.y + scale(words['Y']);
            }
            if (words.count('Z')) {
                target.z = pos.z + scale(words['Z']);
            }
        }

        if (!havePos) {
            pos = target;
            havePos = true;
            appendIfMoved(result.waypoints, pos);
            if ((thisMotion == 0 || thisMotion == 1) &&
                pointDistance(pos, target) <= kEps) {
                ++result.moveCount;
                continue;
            }
        }

        if (thisMotion == 0 || thisMotion == 1) {
            appendIfMoved(result.waypoints, target, &result.segmentFeedrates, segFeed);
            pos = target;
            ++result.moveCount;
            continue;
        }

        if (thisMotion == 2 || thisMotion == 3) {
            const bool clockwise = (thisMotion == 2);
            double iOff = 0.0;
            double jOff = 0.0;
            double kOff = 0.0;
            std::string parseErr;
            if (words.count('R')) {
                if (!radiusToIjk(pos, target, scale(words['R']), clockwise, iOff, jOff,
                                 &parseErr)) {
                    result.error = "Block " + std::to_string(bi + 1) + ": " + parseErr;
                    result.waypoints.clear();
                    result.segmentFeedrates.clear();
                    return result;
                }
            } else {
                if (words.count('I')) {
                    iOff = scale(words['I']);
                }
                if (words.count('J')) {
                    jOff = scale(words['J']);
                }
                if (words.count('K')) {
                    kOff = scale(words['K']);
                }
                if (std::abs(iOff) < kEps && std::abs(jOff) < kEps &&
                    std::abs(kOff) < kEps) {
                    result.error = "Block " + std::to_string(bi + 1) +
                                   ": G2/G3 requires IJK or R";
                    result.waypoints.clear();
                    result.segmentFeedrates.clear();
                    return result;
                }
            }

            auto samples = sampleArc(pos, target, iOff, jOff, kOff, clockwise,
                                     cfg_.arcChordTol, cfg_.arcMinSegments);
            for (const auto& p : samples) {
                appendIfMoved(result.waypoints, p, &result.segmentFeedrates, segFeed);
            }
            pos = target;
            ++result.moveCount;
            continue;
        }
    }

    if (result.waypoints.size() < 2) {
        result.error = "NC program produced fewer than 2 waypoints";
        result.waypoints.clear();
        result.segmentFeedrates.clear();
        return result;
    }

    // 段进给与折线边一一对应；若因去重点导致长度不齐，用 0 补齐（回退 maxVel）。
    if (result.segmentFeedrates.size() + 1 != result.waypoints.size()) {
        result.segmentFeedrates.resize(result.waypoints.size() - 1, 0.0);
    }
    return result;
}

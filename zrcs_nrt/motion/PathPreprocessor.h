#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include "VelocityPlanner3D.h"

class PathPreprocessor {
public:
    // ── 工业方案：直线段 + 局部角点 Bezier 过渡 ──────────────────────
    std::vector<Point3D> processWithCornerBlend(
        const std::vector<Point3D>& raw, double stepSize, double cornerTol)
    {
        const size_t N = raw.size();
        if (N < 2) return raw;
        if (N == 2) return sampleStraight(raw[0], raw[1], stepSize);

        // Phase 1: 计算每个内部角点的过渡参数
        struct CornerBlend {
            bool active = false;
            double d = 0;
            Point3D B0, B1, B2, B3;
        };
        std::vector<CornerBlend> corners(N);

        for (size_t i = 1; i < N - 1; ++i) {
            Point3D vIn  = vecNorm(vecSub(raw[i], raw[i - 1]));
            Point3D vOut = vecNorm(vecSub(raw[i + 1], raw[i]));
            double cosTheta = vecDot(vIn, vOut);
            double theta = std::acos(std::clamp(cosTheta, -1.0, 1.0));

            if (theta < 0.01) continue; // 几乎共线，跳过

            double alpha = theta / 2.0;
            double cosA = std::cos(alpha);
            double dTol = (1.0 - cosA) > 1e-9
                          ? cornerTol * cosA / (1.0 - cosA)
                          : 1e6;

            double lIn  = vecLen(vecSub(raw[i], raw[i - 1]));
            double lOut = vecLen(vecSub(raw[i + 1], raw[i]));
            double dMax = 0.5 * std::min(lIn, lOut);
            double d    = std::min(dTol, dMax);

            if (d < stepSize) continue; // 过渡区太小

            constexpr double k = 0.5;
            corners[i].active = true;
            corners[i].d  = d;
            corners[i].B0 = vecAdd(raw[i], vecScale(vIn, -d));
            corners[i].B3 = vecAdd(raw[i], vecScale(vOut, d));
            corners[i].B1 = vecAdd(corners[i].B0, vecScale(vIn, k * d));
            corners[i].B2 = vecAdd(corners[i].B3, vecScale(vOut, -k * d));
        }

        // Phase 2: 沿折线行走，输出采样点
        std::vector<Point3D> result;
        result.push_back(raw[0]);

        for (size_t i = 0; i < N - 1; ++i) {
            Point3D segStart = (i > 0 && corners[i].active)
                               ? corners[i].B3 : raw[i];
            Point3D segEnd   = (i + 1 < N - 1 && corners[i + 1].active)
                               ? corners[i + 1].B0 : raw[i + 1];

            // 直线段只保留端点，不做中间密集采样
            double straightLen = vecLen(vecSub(segEnd, segStart));
            if (straightLen > stepSize * 0.5) {
                result.push_back(segEnd);
            }

            // 采样角点 Bezier 过渡
            if (i + 1 < N - 1 && corners[i + 1].active) {
                double arcEst = estimateBezierLen(corners[i + 1]);
                int nSamples = std::max(2, (int)std::ceil(arcEst / stepSize));
                for (int j = 0; j <= nSamples; ++j) {
                    double t = (double)j / nSamples;
                    result.push_back(evalCubicBezier(corners[i + 1], t));
                }
            } else if (i + 1 == N - 1) {
                result.push_back(raw[N - 1]);
            }
        }

        // Phase 3: 去重
        deduplicate(result, stepSize * 0.1);

        return result;
    }

    // ── 旧方案（备选）：自然三次样条 ────────────────────────────────
    std::vector<Point3D> processWithSpline(const std::vector<Point3D>& rawPath, double stepSize) {
        if (rawPath.size() < 3) return rawPath;

        std::vector<double> s = calculateArcLength(rawPath);
        auto coeffsX = solveSpline(s, getAxisVector(rawPath, 'x'));
        auto coeffsY = solveSpline(s, getAxisVector(rawPath, 'y'));
        auto coeffsZ = solveSpline(s, getAxisVector(rawPath, 'z'));

        std::vector<Point3D> result;
        double totalLen = s.back();
        for (double t = 0; t <= totalLen; t += stepSize) {
            result.push_back({
                interpolate(s, coeffsX, t),
                interpolate(s, coeffsY, t),
                interpolate(s, coeffsZ, t)
            });
        }
        return result;
    }

private:
    // ── 3D 向量辅助函数 ─────────────────────────────────────────────
    static Point3D vecSub(const Point3D& a, const Point3D& b) {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }
    static Point3D vecAdd(const Point3D& a, const Point3D& b) {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }
    static Point3D vecScale(const Point3D& v, double s) {
        return {v.x * s, v.y * s, v.z * s};
    }
    static double vecDot(const Point3D& a, const Point3D& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    static double vecLen(const Point3D& v) {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }
    static Point3D vecNorm(const Point3D& v) {
        double len = vecLen(v);
        if (len < 1e-12) return {0, 0, 0};
        return {v.x / len, v.y / len, v.z / len};
    }

    // ── Bezier 辅助 ─────────────────────────────────────────────────
    struct CornerBlend;

    template<typename CB>
    static Point3D evalCubicBezier(const CB& c, double t) {
        double u = 1.0 - t;
        double u2 = u * u, u3 = u2 * u;
        double t2 = t * t, t3 = t2 * t;
        return {
            u3 * c.B0.x + 3 * u2 * t * c.B1.x + 3 * u * t2 * c.B2.x + t3 * c.B3.x,
            u3 * c.B0.y + 3 * u2 * t * c.B1.y + 3 * u * t2 * c.B2.y + t3 * c.B3.y,
            u3 * c.B0.z + 3 * u2 * t * c.B1.z + 3 * u * t2 * c.B2.z + t3 * c.B3.z
        };
    }

    template<typename CB>
    static double estimateBezierLen(const CB& c) {
        double chord = vecLen(vecSub(c.B3, c.B0));
        double poly  = vecLen(vecSub(c.B1, c.B0))
                     + vecLen(vecSub(c.B2, c.B1))
                     + vecLen(vecSub(c.B3, c.B2));
        return 0.5 * (chord + poly);
    }

    // ── 直线采样 ────────────────────────────────────────────────────
    static std::vector<Point3D> sampleStraight(const Point3D& a, const Point3D& b, double stepSize) {
        std::vector<Point3D> result;
        Point3D diff = vecSub(b, a);
        double len = vecLen(diff);
        if (len < 1e-9) {
            result.push_back(a);
            result.push_back(b);
            return result;
        }
        Point3D dir = vecScale(diff, 1.0 / len);
        for (double s = 0; s <= len; s += stepSize) {
            result.push_back(vecAdd(a, vecScale(dir, s)));
        }
        // 确保终点
        if (vecLen(vecSub(result.back(), b)) > stepSize * 0.1) {
            result.push_back(b);
        }
        return result;
    }

    // ── 去重 ────────────────────────────────────────────────────────
    static void deduplicate(std::vector<Point3D>& pts, double minDist) {
        if (pts.size() < 2) return;
        std::vector<Point3D> clean;
        clean.push_back(pts[0]);
        for (size_t i = 1; i < pts.size(); ++i) {
            if (vecLen(vecSub(pts[i], clean.back())) >= minDist) {
                clean.push_back(pts[i]);
            }
        }
        pts = std::move(clean);
    }

    // ── 自然三次样条（旧方案辅助）───────────────────────────────────
    struct SplineCoeffs {
        std::vector<double> a, b, c, d;
    };

    SplineCoeffs solveSpline(const std::vector<double>& x, const std::vector<double>& y) {
        int n = x.size() - 1;
        std::vector<double> h(n), alpha(n), l(n + 1), mu(n + 1), z(n + 1), c(n + 1), b(n), d(n);

        for (int i = 0; i < n; ++i) h[i] = x[i + 1] - x[i];
        for (int i = 1; i < n; ++i)
            alpha[i] = (3.0 / h[i]) * (y[i + 1] - y[i]) - (3.0 / h[i - 1]) * (y[i] - y[i - 1]);

        l[0] = 1.0; mu[0] = 0.0; z[0] = 0.0;
        for (int i = 1; i < n; ++i) {
            l[i] = 2.0 * (x[i + 1] - x[i - 1]) - h[i - 1] * mu[i - 1];
            mu[i] = h[i] / l[i];
            z[i] = (alpha[i] - h[i - 1] * z[i - 1]) / l[i];
        }
        l[n] = 1.0; z[n] = 0.0; c[n] = 0.0;
        for (int j = n - 1; j >= 0; --j) {
            c[j] = z[j] - mu[j] * c[j + 1];
            b[j] = (y[j + 1] - y[j]) / h[j] - h[j] * (c[j + 1] + 2.0 * c[j]) / 3.0;
            d[j] = (c[j + 1] - c[j]) / (3.0 * h[j]);
        }

        std::vector<double> a(y.begin(), y.end() - 1);
        return {a, b, c, d};
    }

    double interpolate(const std::vector<double>& x, const SplineCoeffs& co, double t) {
        auto it = std::upper_bound(x.begin(), x.end(), t) - 1;
        int i = std::distance(x.begin(), it);
        i = std::max(0, std::min(i, (int)co.a.size() - 1));
        double dx = t - x[i];
        return co.a[i] + co.b[i] * dx + co.c[i] * dx * dx + co.d[i] * dx * dx * dx;
    }

    std::vector<double> calculateArcLength(const std::vector<Point3D>& path) {
        std::vector<double> s = {0.0};
        for (size_t i = 1; i < path.size(); ++i) {
            double d = std::sqrt(std::pow(path[i].x - path[i-1].x, 2) +
                                std::pow(path[i].y - path[i-1].y, 2) +
                                std::pow(path[i].z - path[i-1].z, 2));
            s.push_back(s.back() + d);
        }
        return s;
    }

    std::vector<double> getAxisVector(const std::vector<Point3D>& path, char axis) {
        std::vector<double> r;
        for (auto& p : path) r.push_back(axis == 'x' ? p.x : (axis == 'y' ? p.y : p.z));
        return r;
    }
};

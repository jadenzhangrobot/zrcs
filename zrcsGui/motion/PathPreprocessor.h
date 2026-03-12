#pragma once
#include <vector>
#include <cmath>
#include "VelocityPlanner3D.h"

class PathPreprocessor {
public:
    // 将离散点路径拟合为平滑的三次样条曲线，并按步长重采样
    std::vector<Point3D> processWithSpline(const std::vector<Point3D>& rawPath, double stepSize) {
        if (rawPath.size() < 3) return rawPath;

        // 分别对 X, Y, Z 关于弧长 s 进行独立拟合
        std::vector<double> s = calculateArcLength(rawPath);
        auto coeffsX = solveSpline(s, getAxisVector(rawPath, 'x'));
        auto coeffsY = solveSpline(s, getAxisVector(rawPath, 'y'));
        auto coeffsZ = solveSpline(s, getAxisVector(rawPath, 'z'));

        // 重采样
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
    struct SplineCoeffs {
        std::vector<double> a, b, c, d;
    };

    // 核心算法：求解自然三次样条（Natural Cubic Spline）
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

    // 辅助函数：计算累计弧长作为样条的参数轴
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
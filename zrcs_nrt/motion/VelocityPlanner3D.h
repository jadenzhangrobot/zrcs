#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <fstream>  // 必须包含这个头文件
// ==========================================
// 1. 修改点结构体为 3D
// ==========================================
struct Point3D {
    double x;
    double y;
    double z;
};

struct WayPoint {
    Point3D pos;            // 3D 坐标
    double dist_to_next;    // 到下一个点的空间距离 (3D 距离)
    double max_v;           // 规划速度
    
    WayPoint(double x, double y, double z) : pos{x, y, z}, dist_to_next(0), max_v(0) {}
};

class VelocityPlanner3D {
private:
    std::vector<WayPoint> path;

    double max_vel_global;
    double max_accel;
    double start_vel;
    double end_vel;
    double corner_tolerance;  // 拐角偏差容限 (mm)

    // ==========================================
    // 2. 修改几何计算函数适配 3D
    // ==========================================

    // 计算 3D 欧几里得距离
    double getDistance(const Point3D& p1, const Point3D& p2) {
        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double dz = p2.z - p1.z; // 新增 Z
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    // 计算 3D 空间向量夹角
    double getCosAngle(const Point3D& prev, const Point3D& curr, const Point3D& next) {
        // 向量 v1 = curr - prev
        double v1x = curr.x - prev.x;
        double v1y = curr.y - prev.y;
        double v1z = curr.z - prev.z;

        // 向量 v2 = next - curr
        double v2x = next.x - curr.x;
        double v2y = next.y - curr.y;
        double v2z = next.z - curr.z;

        double len1 = std::sqrt(v1x*v1x + v1y*v1y + v1z*v1z);
        double len2 = std::sqrt(v2x*v2x + v2y*v2y + v2z*v2z);

        if (len1 < 1e-6 || len2 < 1e-6) return 0.0; // 防止除零
        
        // 3D 点积: x*x + y*y + z*z
        double dot = v1x * v2x + v1y * v2y + v1z * v2z;
        return dot / (len1 * len2);
    }

    // ==========================================
    // 核心算法 (这部分逻辑与 2D 完全通用，无需修改)
    // ==========================================

    void calculateGeometryConstraints() {
        size_t N = path.size();
        path[0].max_v = start_vel;
        path[N - 1].max_v = end_vel;

        for (size_t i = 1; i < N - 1; ++i) {
            double cos_theta = getCosAngle(path[i - 1].pos, path[i].pos, path[i + 1].pos);

            // 向心加速度约束: v_corner = sqrt(a_max * r_tol / (1 - cos_theta))
            double denom = 1.0 - cos_theta;
            if (denom < 1e-6) denom = 1e-6;  // 接近直线，不限速
            double v_corner = std::sqrt(max_accel * corner_tolerance / denom);
            path[i].max_v = std::min(max_vel_global, v_corner);
        }
    }

    void backwardScan() {
        size_t N = path.size();
        for (int i = N - 2; i >= 0; --i) {
            double dist = path[i].dist_to_next;
            double v_next = path[i + 1].max_v;
            double max_reachable_v = std::sqrt(v_next * v_next + 2 * max_accel * dist);
            path[i].max_v = std::min(path[i].max_v, max_reachable_v);
        }
    }

    void forwardScan() {
        size_t N = path.size();
        for (size_t i = 1; i < N; ++i) {
            double dist = path[i - 1].dist_to_next;
            double v_prev = path[i - 1].max_v;
            double max_reachable_v = std::sqrt(v_prev * v_prev + 2 * max_accel * dist);
            path[i].max_v = std::min(path[i].max_v, max_reachable_v);
        }
    }

public:
    VelocityPlanner3D()
        : max_vel_global(100.0), max_accel(100.0), start_vel(0.0), end_vel(0.0),
          corner_tolerance(0.5) {}

    void setConfig(double max_v, double max_a, double start_v = 0.0, double end_v = 0.0,
                   double corner_tol = 0.5) {
        max_vel_global = max_v;
        max_accel = max_a;
        start_vel = start_v;
        end_vel = end_v;
        corner_tolerance = corner_tol;
    }

    // 3. 接口增加 Z 参数
    void addPoint(double x, double y, double z) {
        path.emplace_back(x, y, z);
    }

    void clearPath() {
        path.clear();
    }

    bool plan() {
        if (path.size() < 2) {
            std::cerr << "Error: Path must have at least 2 points." << std::endl;
            return false;
        }

        // 计算 3D 距离
        for (size_t i = 0; i < path.size() - 1; ++i) {
            path[i].dist_to_next = getDistance(path[i].pos, path[i+1].pos);
        }

        calculateGeometryConstraints();
        backwardScan();
        forwardScan();

        return true;
    }

    void printReport(const std::string& filename) const {
        printReport(filename, {});
    }

    void printReport(const std::string& filename, const std::vector<Point3D>& rawPoints) const {
        std::ofstream outFile(filename);

        if (!outFile.is_open()) {
            std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
            return;
        }

        outFile << std::fixed << std::setprecision(4);
        outFile << "Index,X,Y,Z,Distance,VelocityLimit" << std::endl;

        for (size_t i = 0; i < path.size(); ++i) {
            outFile << i << ","
                    << path[i].pos.x << ","
                    << path[i].pos.y << ","
                    << path[i].pos.z << ","
                    << (i < path.size() - 1 ? path[i].dist_to_next : 0.0) << ","
                    << path[i].max_v << "\n";
        }

        if (!rawPoints.empty()) {
            outFile << "RAW_POINTS" << std::endl;
            outFile << "Index,X,Y,Z" << std::endl;
            for (size_t i = 0; i < rawPoints.size(); ++i) {
                outFile << i << ","
                        << rawPoints[i].x << ","
                        << rawPoints[i].y << ","
                        << rawPoints[i].z << "\n";
            }
        }

        outFile.close();
        std::cout << "Successfully saved planning data to: " << filename << std::endl;
    }
};


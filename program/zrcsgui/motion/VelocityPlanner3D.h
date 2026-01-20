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
            
            // 简单的 3D 拐角限速逻辑
            double turn_factor = (cos_theta + 1.0) / 2.0; 
            if(turn_factor < 0.1) turn_factor = 0.1;

            double geometry_limit = max_vel_global * turn_factor;
            path[i].max_v = std::min(max_vel_global, geometry_limit);
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
        : max_vel_global(100.0), max_accel(100.0), start_vel(0.0), end_vel(0.0) {}

    void setConfig(double max_v, double max_a, double start_v = 0.0, double end_v = 0.0) {
        max_vel_global = max_v;
        max_accel = max_a;
        start_vel = start_v;
        end_vel = end_v;
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

    void printReport() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "--- 3D Velocity Planning Report ---" << std::endl;
        std::cout << "Idx\tX\tY\tZ\tDist3D\tVel Limit" << std::endl;
        std::cout << "----------------------------------------------------" << std::endl;
        for (size_t i = 0; i < path.size(); ++i) {
            std::cout << i << "\t" 
                      << (int)path[i].pos.x << "\t" 
                      << (int)path[i].pos.y << "\t" 
                      << (int)path[i].pos.z << "\t" 
                      << (i < path.size()-1 ? path[i].dist_to_next : 0.0) << "\t" 
                      << path[i].max_v << std::endl;
        }
    }
};


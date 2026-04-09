/// @file GalvoFieldCorrection.h
/// @brief 振镜场畸变校正 — 预分配 2D 查找表 + 双线性插值
///
/// f-theta 镜头导致扫描场边缘位置偏移，此类在 init 阶段加载校正表，
/// 在 RT 周期内通过双线性插值做 O(1) 校正。
#pragma once

#include <cmath>
#include <string>
#include <fstream>

class GalvoFieldCorrection {
public:
    static constexpr int kGridSize = 256;

    /// 从校正数据文件加载标定数据 (非 RT，仅在 init 调用)
    /// 文件格式: 二进制，依次为
    ///   fieldMinX, fieldMaxX, fieldMinY, fieldMaxY (4 × double)
    ///   corrX[256][256], corrY[256][256] (2 × 256 × 256 × double)
    bool loadCalibration(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        file.read(reinterpret_cast<char*>(&fieldMinX_), sizeof(double));
        file.read(reinterpret_cast<char*>(&fieldMaxX_), sizeof(double));
        file.read(reinterpret_cast<char*>(&fieldMinY_), sizeof(double));
        file.read(reinterpret_cast<char*>(&fieldMaxY_), sizeof(double));
        file.read(reinterpret_cast<char*>(corrX_), sizeof(corrX_));
        file.read(reinterpret_cast<char*>(corrY_), sizeof(corrY_));

        if (!file.good()) return false;

        cellSizeX_ = (fieldMaxX_ - fieldMinX_) / (kGridSize - 1);
        cellSizeY_ = (fieldMaxY_ - fieldMinY_) / (kGridSize - 1);
        loaded_ = true;
        return true;
    }

    /// 生成单位校正表 (无畸变, 用于测试)
    void loadIdentity(double fieldMin, double fieldMax)
    {
        fieldMinX_ = fieldMinY_ = fieldMin;
        fieldMaxX_ = fieldMaxY_ = fieldMax;
        cellSizeX_ = cellSizeY_ = (fieldMax - fieldMin) / (kGridSize - 1);
        for (int r = 0; r < kGridSize; ++r) {
            for (int c = 0; c < kGridSize; ++c) {
                corrX_[r][c] = 0;
                corrY_[r][c] = 0;
            }
        }
        loaded_ = true;
    }

    /// RT 安全: 对理想坐标做畸变校正，输出修正后坐标
    /// @param idealX, idealY 理想场内坐标 (mm)
    /// @param correctedX, correctedY 校正后坐标 (mm)
    void correct(double idealX, double idealY,
                 double& correctedX, double& correctedY) const
    {
        if (!loaded_) {
            correctedX = idealX;
            correctedY = idealY;
            return;
        }

        // 归一化到网格坐标
        double gx = (idealX - fieldMinX_) / cellSizeX_;
        double gy = (idealY - fieldMinY_) / cellSizeY_;

        // 限制到有效范围
        if (gx < 0) gx = 0;
        if (gx > kGridSize - 1.001) gx = kGridSize - 1.001;
        if (gy < 0) gy = 0;
        if (gy > kGridSize - 1.001) gy = kGridSize - 1.001;

        int ix = static_cast<int>(gx);
        int iy = static_cast<int>(gy);
        double fx = gx - ix;
        double fy = gy - iy;

        // 双线性插值
        double cx = corrX_[iy][ix] * (1 - fx) * (1 - fy)
                   + corrX_[iy][ix + 1] * fx * (1 - fy)
                   + corrX_[iy + 1][ix] * (1 - fx) * fy
                   + corrX_[iy + 1][ix + 1] * fx * fy;

        double cy = corrY_[iy][ix] * (1 - fx) * (1 - fy)
                   + corrY_[iy][ix + 1] * fx * (1 - fy)
                   + corrY_[iy + 1][ix] * (1 - fx) * fy
                   + corrY_[iy + 1][ix + 1] * fx * fy;

        correctedX = idealX + cx;
        correctedY = idealY + cy;
    }

    bool isLoaded() const { return loaded_; }

private:
    double corrX_[kGridSize][kGridSize] = {};
    double corrY_[kGridSize][kGridSize] = {};
    double fieldMinX_ = -50, fieldMaxX_ = 50;
    double fieldMinY_ = -50, fieldMaxY_ = 50;
    double cellSizeX_ = 1, cellSizeY_ = 1;
    bool loaded_ = false;
};

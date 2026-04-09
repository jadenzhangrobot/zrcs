/*
 * @Description: 电子凸轮（固高 GT_SetCamMode / 正运动 CAMBOX）
 *               从轴按凸轮表映射主轴位置，OutputNode 持续运行
 */
#pragma once
#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/NodeFactory.h"
#include <vector>
#include <algorithm>

class CamMove : public zrcsSystem::CmdNode
{
private:
    int mainAxisId_;
    int slaveAxisId_;
    int tableId_;

    // 凸轮表: masterPos -> slavePos 映射点
    struct CamPoint
    {
        double masterPos;
        double slavePos;
    };
    std::vector<CamPoint> camTable_;

    double interpolate(double masterPos) const
    {
        if (camTable_.empty()) return 0;
        if (masterPos <= camTable_.front().masterPos) return camTable_.front().slavePos;
        if (masterPos >= camTable_.back().masterPos) return camTable_.back().slavePos;

        for (size_t i = 1; i < camTable_.size(); i++)
        {
            if (masterPos <= camTable_[i].masterPos)
            {
                double t = (masterPos - camTable_[i-1].masterPos) /
                           (camTable_[i].masterPos - camTable_[i-1].masterPos);
                return camTable_[i-1].slavePos + t * (camTable_[i].slavePos - camTable_[i-1].slavePos);
            }
        }
        return camTable_.back().slavePos;
    }

public:
    CamMove() : mainAxisId_(0), slaveAxisId_(0), tableId_(0)
    {
        std::strcpy(nodeName_, "CamMove");
    }

    void init() override;
    void run(void) override;
    void exit(void) override {}
};

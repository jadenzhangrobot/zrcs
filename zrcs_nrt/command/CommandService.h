#pragma once
/**
 * @file CommandService.h
 * @brief 普通 RT 运动/轴命令：经 RtBridge 写入 cmdQueue。
 */

#include <string>
#include <utility>
#include <vector>

#include "rtBridge/RtBridge.h"

class CommandService {
public:
    explicit CommandService(RtBridge* bridge);

    /// 提交命令，返回给上位机的回复串（"OK" / "ERROR: ..."）。
    std::string submit(const std::string& name, const std::vector<double>& args);

    /// 与 RtBridge 相同语义，供需要序号的调用方使用。
    std::pair<RtBridge::SendResult, uint32_t> submitWithSeq(const std::string& name,
                                                             const std::vector<double>& args);

    RtBridge* bridge() const { return bridge_; }

private:
    RtBridge* bridge_;
};

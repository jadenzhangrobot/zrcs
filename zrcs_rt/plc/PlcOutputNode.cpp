#include "plc/PlcOutputNode.h"

#include <cstring>

#include "plc/PlcProgram.h"
#include "system/node/NodeFactory.h"

PlcOutputNode::PlcOutputNode()
{
    std::strncpy(nodeName_, "PlcOutputNode", sizeof(nodeName_) - 1);
    nodeName_[sizeof(nodeName_) - 1] = '\0';
}

void PlcOutputNode::onRegistered()
{
    lastRunning_ = false;
}

void PlcOutputNode::init()
{
    // PLC 实例由 PlcLogicNode 在 RT 启动前统一初始化。
}

void PlcOutputNode::run()
{
    if (shm()->taskSched.load(std::memory_order_acquire) != zrcs::TaskScheduling::RUN) {
        return;
    }

    // 在此添加 PLC 输出 -> RT 变量/IO 的映射，读取本拍扫描结果。
    const bool running = __GET_VAR(matiec::ZRCS__DEMO.Q_RUNNING, ) != 0;
    if (running != lastRunning_) {
        lastRunning_ = running;
        INFO_PRINT("[PLC] running=%u scanAlive=%u tSec=%d.%09d\n",
                   static_cast<unsigned>(running),
                   static_cast<unsigned>(__GET_VAR(matiec::ZRCS__DEMO.Q_SCANALIVE, )),
                   matiec::__CURRENT_TIME.tv_sec, matiec::__CURRENT_TIME.tv_nsec);
    }
    // TODO(P2): 在物理 IO 映射确定后，把 PLC 输出写入 controller_->ios_。
}

REGISTER_PERIODIC(PlcOutputNode, OUTPUT, 110);

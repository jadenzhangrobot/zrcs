/*
 * @Description: 延时等待命令
 */
#include "command/Wait.h"

void Wait::init()
{
    waitTimeMs_ = command_->args[WaitTimeMs];
    startCount_ = nodeCount_;
}

void Wait::run(void)
{
    double elapsedMs = (nodeCount_ - startCount_) * cycletime;
    if (elapsedMs >= waitTimeMs_)
    {
        setCmdStatus(zrcsSystem::CmdStatus::EXIT);
    }
    nodeCount_++;
}

void Wait::exit(void) {}

REGISTERCMD(Wait);

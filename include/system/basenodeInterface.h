/**
 * @copyright Copyright(c) 2024 Glroad Corporation
 * @filename: basenodeInterface.h
 * @brief: Base node interface definitions for ZRCS system
 * @author: zhangyongjing@oetsky.com
 * @createdate: 2024-01-08
 */
#ifndef BASENODE_INTERFACE_H_
#define BASENODE_INTERFACE_H_
#include "common/config/cmdArgs.h"
#include "common/sharedMemory/rtProcess.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include <atomic>
#include <cstdint>
#include <string>
#include "common/rtLog.h" 
#include "common/config/parameter.h"

namespace zrcsSystem {

class Basenode {
public:
    uint64_t nodeCount = 0;
    char nodeName[32]={};
    
 
   // cmdline::parser port_input;
    //命令参数
    std::string cmdParam={};
    ZrcsHardware::Controller* control;
    RTProcess *rtProcess;
    Command* command=nullptr;

    Basenode() : control(nullptr), rtProcess(nullptr)
    {
    }
    virtual ~Basenode() = default;
    
    void registered(ZrcsHardware::Controller* ct , RTProcess *rtProcess_,Command* command_) 
    {
        control = ct;
        rtProcess=rtProcess_;
        command=command_;
    }
    void registered(ZrcsHardware::Controller* ct , RTProcess *rtProcess_) 
    {
        control = ct;
        rtProcess=rtProcess_;
    }
    //获取节点名字
    std::string getNodeNAME(void)
    {
        return nodeName;
    }
    std::uint64_t getNodeCount()
    {
      return nodeCount;
    }
};
enum class  CmdStatus {
    START,       // 节点已创建
    INIT,      // 非实时初始化中
    EXECUTING,     // 核心逻辑执行中
    EXIT,      // 非实时退出中
    COMPLETED,     // 执行成功完成
    FAILED         // 执行失败
};


class CmdNode : public Basenode {
public:
    std::atomic<CmdStatus> cmdStatus{CmdStatus::INIT};
    
    virtual ~CmdNode() = default;
    
    // 非实时初始化
    virtual void init() = 0;
    virtual void run()=0;
       // 非实时退出
    virtual void exit() = 0;
  void execute()
  {
           switch (cmdStatus.load())
                {              
                  case CmdStatus::INIT:
                        init();
                        INFO_PRINT("%s 初始化成功\n",nodeName);
                        cmdStatus.store(CmdStatus::EXECUTING,std::memory_order_release);             
                        break;
                  case CmdStatus::EXECUTING:
                       run();
                       break;                
                  case CmdStatus::EXIT:
                        exit();
                        INFO_PRINT("%s 执行成功\n",nodeName);
                        cmdStatus.store(CmdStatus::COMPLETED,std::memory_order_release);                
                        break;                  
                  case  CmdStatus::FAILED:
                        INFO_PRINT("%s 执行失败\n",nodeName); 
                        taskScheduling.store(TaskScheduling::ERROR,std::memory_order_release);                    
                    break;                
                  default:                                     
                    break;
                }
  } 
    // 获取一次性节点特定状态
    //     // 获取一次性节点特定状态
    CmdStatus getCmdStatus() const noexcept {
        return cmdStatus.load(std::memory_order_acquire);
    }
    
    // 设置一次性节点状态
    void setCmdStatus(CmdStatus status) {
        cmdStatus.store(status, std::memory_order_release);
    }
};


enum class NodeStatus 
{
    RTINIT,       // 实时初始化中
    EXECUTING,       // 持续运行中
    FAILED         // 运行失败
};

// // 持久性节点：持续运行的节点
class OutputNode : public Basenode 
{
public:
    std::atomic<NodeStatus> nodeStatus{NodeStatus::RTINIT};
    OutputNode() = default;
    virtual ~OutputNode() = default;
    virtual void init() = 0;
    virtual void run()=0;
   
    void execute()
    {
        switch (nodeStatus.load())
                {              
                  case NodeStatus::RTINIT:
                        init();
                        INFO_PRINT("%s 初始化成功\n",nodeName);
                        nodeStatus.store(NodeStatus::EXECUTING,std::memory_order_release);             
                        break;
                  case NodeStatus::EXECUTING:
                        run();
                       break;                          
                  case  NodeStatus::FAILED: 
                        INFO_PRINT("%s 执行失败\n",nodeName);  
                    break;                
                  default:                                     
                    break;
                }
    }
    NodeStatus getNodeStatus() 
    {
        return nodeStatus.load(std::memory_order_acquire);
    }
    void setNodeStatus(NodeStatus status) 
    {
        nodeStatus.store(status, std::memory_order_release);
    }
};
class InputNode : public Basenode {
public:
    
    InputNode() = default;
    virtual ~InputNode() = default;
    

     virtual void execute()=0;
};



} // namespace zrcsSystem
#endif

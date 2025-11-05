/**
 * @copyright Copyright(c) 2024 Glroad Corporation
 * @filename: basenodeInterface.h
 * @brief: Base node interface definitions for ZRCS system
 * @author: zhangyongjing@oetsky.com
 * @createdate: 2024-01-08
 */
#ifndef BASENODE_INTERFACE_H_
#define BASENODE_INTERFACE_H_
#include "cmdline.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include <atomic>
#include <cstdint>
#include <string>
#include "common/rtLog.h" 
#include "common/sharedMemory/rtProcess.h"
#include "common/sharedMemory/rtProcess.h"
#include "common/config/parameter.h"

namespace zrcsSystem {

class Basenode {
public:
    uint64_t nodeCount = 0;
    std::string nodeName;
    
 
    cmdline::parser port_input;
    //命令参数
    std::string cmdParam={};
    ZrcsHardware::Controller* control;
    RTProcess *rtProcess;
    Basenode() : control(nullptr) {}
    virtual ~Basenode() = default;
    
    // 纯虚函数：执行节点逻辑
    virtual void execute() = 0;
    
    // 处理失败状态
    void failure();
    
    // 获取任务状态（线程安全）
    // virtual BaseNodeStatus GetTaskState() const noexcept {
    //     return nodeStatus.load(std::memory_order_acquire);
    // }
    
    // 注册控制器
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
    // 推送命令参数
    void pushCmdArgs(const std::string& cmdargs) {
        if (cmdParam.empty()) 
        {
           cmdParam=cmdargs;
        }
    }
    //移除节点参数
    void popCmdArgs()
    {
      if (!cmdParam.empty()) 
      {
          cmdParam.clear();
      }
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

// 一次性节点：执行一次后退出
// class CmdNode : public Basenode {
// public:
//     std::atomic<CmdStatus> cmdStatus{CmdStatus::START};
    
//     virtual ~CmdNode() = default;
    
//     // 非实时初始化
//     virtual void init() = 0;
//     virtual void run()=0;
//        // 非实时退出
//     virtual void exit() = 0;
//     void execute()override
//     {
//           switch (cmdStatus.load())
//                 {                                   
//                   case CmdStatus::EXECUTING:
//                        run();
//                        break;                   
//                   default:                    
//                        break;
//                 }
//           nodeCount++;
//     }
//   void executeNrt()
//   {
//            switch (cmdStatus.load())
//                 {              
//                   case CmdStatus::INIT:
//                         init();
//                         cmdStatus.store(CmdStatus::EXECUTING,std::memory_order_release);             
//                         break;                
//                   case CmdStatus::EXIT:
//                         exit();
//                         INFO_PRINT("%s 执行成功\n",nodeName.c_str());
//                         cmdStatus.store(CmdStatus::COMPLETED,std::memory_order_release);                
//                         break;                  
//                   case  CmdStatus::FAILED: 
//                         INFO_PRINT("%s 执行失败\n",nodeName.c_str());  
//                     break;                
//                   default:                                     
//                     break;
//                 }
//   } 
//     // 获取一次性节点特定状态
//     CmdStatus getCmdStatus() const noexcept {
//         return cmdStatus.load(std::memory_order_acquire);
//     }
    
//     // 设置一次性节点状态
//     void setCmdStatus(CmdStatus status) {
//         cmdStatus.store(status, std::memory_order_release);
//     }
// };
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
                        cmdStatus.store(CmdStatus::EXECUTING,std::memory_order_release);             
                        break;
                  case CmdStatus::EXECUTING:
                       run();
                       break;                
                  case CmdStatus::EXIT:
                        exit();
                        INFO_PRINT("%s 执行成功\n",nodeName.c_str());
                        cmdStatus.store(CmdStatus::COMPLETED,std::memory_order_release);                
                        break;                  
                  case  CmdStatus::FAILED: 
                        INFO_PRINT("%s 执行失败\n",nodeName.c_str());  
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
    CREATED,
    RTINIT,       // 实时初始化中
    EXECUTING,       // 持续运行中
    RTEXIT,      // 正在停止中 
    FAILED         // 运行失败
};

// 持久性节点：持续运行的节点
class OutputNode : public Basenode {
public:
    std::atomic<NodeStatus> nodeStatus{NodeStatus::CREATED};
    
    OutputNode() = default;
    virtual ~OutputNode() = default;
    
    // 实时初始化
    virtual void init() = 0;
    
    virtual void run()=0;
    // 实时退出
    virtual void exit() = 0;

     void execute()override
    {
          switch (nodeStatus.load())
                {          
                  case NodeStatus::CREATED:
                       setStatus(NodeStatus::RTINIT);
                       break;
                  case NodeStatus::RTINIT:
                       init();
                       setStatus(NodeStatus::EXECUTING);                   
                       break;
                  case NodeStatus::EXECUTING:
                       run();
                       break;
                  case NodeStatus::RTEXIT:
                       exit();
                       setStatus(NodeStatus::RTINIT);   
                       break;
                  case NodeStatus::FAILED:
                       break;
                  default:                    
                       break;
                }
           nodeCount++;
    }
    // 获取持久性节点特定状态
    NodeStatus getStatus() const noexcept {
        return nodeStatus.load(std::memory_order_acquire);
    }
    
    // 设置持久性节点状态
    void setStatus(NodeStatus status) {
         nodeStatus.store(status, std::memory_order_release);
    }
};
class InputNode : public Basenode {
public:
    std::atomic<NodeStatus> nodeStatus{NodeStatus::CREATED};
    
    InputNode() = default;
    virtual ~InputNode() = default;
    
    // 实时初始化
    virtual void init() = 0;
    
    virtual void run()=0;
    // 实时退出
    virtual void exit() = 0;

     void execute()override
    {
          switch (nodeStatus.load())
                {          
                  case NodeStatus::CREATED:
                       setStatus(NodeStatus::RTINIT);
                       break;
                  case NodeStatus::RTINIT:
                       init();
                       setStatus(NodeStatus::EXECUTING);                   
                       break;
                  case NodeStatus::EXECUTING:
                       run();
                       break;
                  case NodeStatus::RTEXIT:
                       exit();
                       setStatus(NodeStatus::RTINIT);   
                       break;
                  case NodeStatus::FAILED:
                       break;
                  default:                    
                       break;
                }
           nodeCount++;
    }
    // 获取持久性节点特定状态
    NodeStatus getStatus() const noexcept {
        return nodeStatus.load(std::memory_order_acquire);
    }
    
    // 设置持久性节点状态
    void setStatus(NodeStatus status) {
         nodeStatus.store(status, std::memory_order_release);
    }
};



} // namespace zrcsSystem
#endif

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
namespace zrcsSystem {

class Basenode {
public:
    uint64_t nodeCount = 0;
    std::string nodeName;
    
 
    cmdline::parser port_input;
    //命令参数
    std::string cmdParam={};
    ZrcsHardware::Controller* control;
    
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
    void registered(ZrcsHardware::Controller* ct) 
    {
        control = ct;
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
    
    // 设置任务状态（线程安全）
    // void SetTaskState(BaseNodeStatus ns) {
    //     nodeStatus.store(ns, std::memory_order_release);
    // }
};
enum class OneShotNodeStatus {
    START,       // 节点已创建
    INIT,      // 非实时初始化中
    EXECUTING,     // 核心逻辑执行中
    EXIT,      // 非实时退出中
    COMPLETED,     // 执行成功完成
    FAILED         // 执行失败
};

// 一次性节点：执行一次后退出
class OneShotNode : public Basenode {
public:
    std::atomic<OneShotNodeStatus> oneShotStatus{OneShotNodeStatus::START};
    
    virtual ~OneShotNode() = default;
    
    // 非实时初始化
    virtual void init() = 0;
    virtual void run()=0;
       // 非实时退出
    virtual void exit() = 0;
    void execute()override
    {
          switch (oneShotStatus.load())
                {               
                  case OneShotNodeStatus::EXECUTING:
                       run();
                       break;                   
                  default:                    
                       break;
                }
          nodeCount++;
    }
  void executeNrt()
  {
           switch (oneShotStatus.load())
                {              
                  case OneShotNodeStatus::INIT:
                        init();
                        oneShotStatus.store(OneShotNodeStatus::EXECUTING,std::memory_order_release);             
                        break;                
                  case OneShotNodeStatus::EXIT:
                        exit();
                        INFO_PRINT("%s 执行成功\n",nodeName.c_str());
                        oneShotStatus.store(OneShotNodeStatus::COMPLETED,std::memory_order_release);                
                        break;                  
                  case  OneShotNodeStatus::FAILED: 
                        INFO_PRINT("%s 执行失败\n",nodeName.c_str());  
                    break;                
                  default:                                     
                    break;
                }
  } 
    // 获取一次性节点特定状态
    OneShotNodeStatus GetOneShotStatus() const noexcept {
        return oneShotStatus.load(std::memory_order_acquire);
    }
    
    // 设置一次性节点状态
    void SetOneShotStatus(OneShotNodeStatus status) {
        oneShotStatus.store(status, std::memory_order_release);
    }
};


enum class PersistentNodeStatus {
    CREATED,
    RTINIT,       // 实时初始化中
    EXECUTING,       // 持续运行中
    RTEXIT,      // 正在停止中 
    FAILED         // 运行失败
};

// 持久性节点：持续运行的节点
class PersistentNode : public Basenode {
public:
    std::atomic<PersistentNodeStatus> persistentStatus{PersistentNodeStatus::CREATED};
    
    PersistentNode() = default;
    virtual ~PersistentNode() = default;
    
    // 实时初始化
    virtual void init() = 0;
    
    virtual void run()=0;
    // 实时退出
    virtual void exit() = 0;

     void execute()override
    {
          switch (GetPersistentStatus())
                {          
                  case PersistentNodeStatus::CREATED:
                       SetPersistentStatus(PersistentNodeStatus::RTINIT);
                       break;
                  case PersistentNodeStatus::RTINIT:
                       init();
                       SetPersistentStatus(PersistentNodeStatus::EXECUTING);                   
                       break;
                  case PersistentNodeStatus::EXECUTING:
                       run();
                       break;
                  case PersistentNodeStatus::RTEXIT:
                       break;
                  case PersistentNodeStatus::FAILED:
                       break;
                  default:                    
                       break;
                }
           nodeCount++;
    }
    // 获取持久性节点特定状态
    PersistentNodeStatus GetPersistentStatus() const noexcept {
        return persistentStatus.load(std::memory_order_acquire);
    }
    
    // 设置持久性节点状态
    void SetPersistentStatus(PersistentNodeStatus status) {
         persistentStatus.store(status, std::memory_order_release);
    }
};



} // namespace zrcsSystem
#endif

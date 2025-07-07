/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:basenode.h
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-08
 */
#ifndef BASEFUN_H_
#define BASEFUN_H_
#include "cmdline.h"
#include <cstdint>
#include <string>
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include <queue>
#include <memory_resource>
#include <atomic> 
namespace zrcsSystem {

class Basenode {
public:
  uint64_t node_count = 0;
  std::string node_name;
  enum  NodeStatus
  {
    START,
    IDLE,    //表示实时线程处于闲暇状态,没有加载任何主件
    NRTINIT,
    RTINIT,    //表示实时组件初始化状态
    EXCUTERT, //表示实时线程正执行任务
    RTEXIT,    //表示实时线程再执行退出参数保存
    NRTEXIT,   // 
    FAILURE, //表示执行错误
  };
  std::atomic<NodeStatus> nodeStatus{NodeStatus::START};
  cmdline::parser port_input;
  std::queue<std::string> cmdParam;
  ZrcsHardware::Controller* control;
  Basenode()
  {};
  virtual ~Basenode() = default;
  virtual void nrtInit(void)=0;

  virtual void rtInit(void) = 0;

  virtual void excuteRt(void) = 0;

  virtual void rtExit(void)=0;
  virtual void nrtExit(void)=0;
   void failure(void);
  virtual NodeStatus GetTaskState() const noexcept { return nodeStatus.load(std::memory_order_acquire); }

  void registered( ZrcsHardware::Controller* ct)
  {
        control=ct;
  }
 void PushCmdArgs(std::string cmdargs)
  {
        cmdParam.push(cmdargs);
  };
 
  void SetTaskState(NodeStatus ns) noexcept
  {
    nodeStatus.store(ns, std::memory_order_release);
  }
};
} // namespace zrcs_system
#endif

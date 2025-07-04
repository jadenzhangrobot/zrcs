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
namespace zrcsSystem {

class Basenode {
public:
  uint64_t node_count = 0;
  std::string node_name;
  enum NodeStatus
  {
    START,
    IDLE,    //表示实时线程处于闲暇状态,没有加载任何主件
    NRTINIT,
    RTINIT,    //表示实时组件初始化状态
    EXCUTERt, //表示实时线程正执行任务
    RTEXIT,    //表示实时线程再执行退出参数保存
    NRTEXIT,   // 
    FAILURE, //表示执行错误
  };
  NodeStatus node_status = START;
  cmdline::parser port_input;
  std::queue<std::string> cmdParam;
  ZrcsHardware::Controller* control;
  Basenode()
  {};
  virtual ~Basenode() = default;
  virtual void nrtInit(void)=0;

  virtual void rtinit(void) = 0;

  virtual void excuteRt(void) = 0;

  virtual void rtExit(void)=0;
  virtual void nrtExit(void)=0;
  virtual void failure(void)=0;
  virtual NodeStatus GetTaskState() { return node_status;}

  void registered( ZrcsHardware::Controller* ct)
  {
        control=ct;
  }
 void PushCmdArgs(std::string cmdargs)
  {
        cmdParam.push(cmdargs);
  };
 
  void  SetTaskState(NodeStatus ns)
  {
        node_status=ns;
  }
};
} // namespace zrcs_system
#endif

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
  enum NodeStatus {
    INIT,    //表示实时组件初始化状态
    IDLE,    //表示实时线程处于闲暇状态,没有加载任何主件
    RUNNING, //表示实时线程正执行任务
    EXIT,
    SUCCESS, //表示执行成功状态，这个状态和IDLE状态的区别在于可以接受指令
    FAILURE, //表示执行错误
  };
  NodeStatus node_status = IDLE;
  cmdline::parser port_input;
  std::queue<std::string> cmdParam;
  HWAL::Controller* control;

  
  Basenode()
  {};
  virtual ~Basenode() = default;

  virtual void init(void) = 0;

  virtual void excuteRt(void) = 0;
  virtual void excuteNrt(void) {};
  void PushCmdArgs(std::string cmdargs)
  {
        cmdParam.push(cmdargs);
  };
  virtual void config(void)
  {

  }

  virtual void exit(void)=0;
 
  void registered( HWAL::Controller* ct)
  {
        control=ct;
  }

  virtual NodeStatus GetTaskState() { return node_status; }
  void  SetTaskState(NodeStatus ns)
  {
        node_status=ns;
  }

  // //由idle状态切换到init状态，对实时节点进行初始化
  // virtual bool config() {
  //   if (node_status == SUCCESS) {
  //     node_status = INIT;
  //   } else {
  //     return false;
  //   }
  //   return true;
  // }
  // //状态由init状态切换到running
  // virtual bool run() {
  //   if (node_status == INIT) {
  //     node_status = RUNNING;
  //   } else {
  //     return false;
  //   }
  //   return true;
  // }

  //将指令从失败的状态恢复到接受指令的success状态
  virtual bool recover() {
    if (node_status == FAILURE) {
      node_status = SUCCESS;
    } else {
      return false;
    }
    return true;
  }
  //主要是用于紧急停止
  virtual bool stop() {
    if ((node_status == RUNNING) || (node_status == SUCCESS)) {
      node_status = IDLE;
    } else {
      return false;
    }
    return true;
  }
  //紧急停止后的状态恢复
  virtual bool start() {
    if (node_status == IDLE) {
      node_status = SUCCESS;
    } else {
      return false;
    }
    return true;
  }
  // virtual bool fail()
  // {
  //   if
  //   ((node_status==INIT)||(node_status==RUNNING)||(node_status==RTCOMPLETE))
  //   {
  //     node_status = FAILURE;
  //   } else {
  //     return false;
  //   }
  //   return true;
  // }
  // virtual bool quit()
  // {
  //    if (node_status==RUNNING)
  //    {
  //      node_status=RTCOMPLETE;
  //    }
  //    else {
  //       return false;
  //    }
  //   return  true;
  // }
};
} // namespace zrcs_system
#endif

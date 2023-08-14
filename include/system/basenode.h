/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 11:30:25
 * @LastEditTime: 2023-06-02 11:09:20
 * @Description: 实时系统指令基类
 *
 */
#ifndef BASEFUN_H_
#define BASEFUN_H_
#include <fstream>
namespace zrcs_system {

class Basenode {
public:
  enum NodeStatus {
    INIT,    //表示实时组件初始化状态
    IDLE,    //表示实时线程处于闲暇状态
    RUNNING, //表示实时线程正执行任务
    SUCCESS, //表示执行成功状态，这个状态和IDLE状态的区别在于可以接受指令
    FAILURE  //表示执行错误
  };
  NodeStatus rtnode_status = SUCCESS;
  Basenode(){};
  virtual ~Basenode() = default;

  virtual bool init(void) {return true;}

  virtual void excute_rt(void) {}

  virtual NodeStatus getTaskState() { return rtnode_status;}

  //由idle状态切换到init状态，对实时节点进行初始化
  virtual bool config() {
    if (rtnode_status == SUCCESS) {
      rtnode_status = INIT;
    } else {
      return false;
    }
    return true;
  }
  //状态由init状态切换到running
  virtual bool run() {
    if (rtnode_status == INIT) {
      rtnode_status = RUNNING;
    } else {
      return false;
    }
    return true;
  }

  //将指令从失败的状态恢复到接受指令的success状态
  virtual bool recover() {
    if (rtnode_status == FAILURE) {
      rtnode_status = SUCCESS;
    } else {
      return false;
    }
    return true;
  }
  //主要是用于紧急停止
  virtual bool stop() {
    if (rtnode_status == RUNNING||SUCCESS) {
      rtnode_status = IDLE;
    } else {
      return false;
    }
    return true;
  }
  //紧急停止后的状态恢复
  virtual bool start() {
    if (rtnode_status == IDLE) {
      rtnode_status = SUCCESS;
    } else {
      return false;
    }
    return true;
  }
  virtual bool fail()
  {
    if (rtnode_status == INIT) {
      rtnode_status = SUCCESS;
    } else {
      return false;
    }
    return true;
  }
};
} // namespace zrcs_system
#endif

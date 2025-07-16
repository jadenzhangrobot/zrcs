/**
 * @copyright 版权所有(c)2024 Glroad公司
 * @filename: centre.h
 * @brief: 系统中心控制类，负责任务调度和节点管理
 * @author: zhangyongjing@oetsky.com
 * @createdate: 2024-01-04
 */
#ifndef CENTRE_H_
#define CENTRE_H_
#include "basenodeInterface.h"
#include "cmdline.h"
#include "command/Cmdhead.h"
#include "common/Shared memory/rtProcess.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include "controller/rtos/linux.h"
#include "nodeCommunication.h"
#include "nodeFactory.h"
#include <array>
#include <mutex>
#include <thread>
#include <vector>

namespace zrcsSystem {
/**
 * @brief 命令队列类，用于线程安全的命令传递
 */
class CmdQueue {
  std::mutex cmdMutex;              // 命令队列互斥锁
  std::queue<std::string> cmdQueue; // 命令队列
public:
  /**
   * @brief 构造函数
   */
  CmdQueue() {}
  /**
   * @brief 写入命令到队列
   * @param cmd 要写入的命令字符串
   */
  void writeCmd(std::string &cmd) {
    std::lock_guard<std::mutex> lock(cmdMutex);
    cmdQueue.push(cmd);
  }
  /**
   * @brief 从队列读取命令
   * @param cmd 输出参数，读取到的命令
   * @return 0表示成功，-1表示队列为空
   */
  int cmdRead(std::string &cmd) {
    if (!cmdQueue.empty()) {
      std::lock_guard<std::mutex> lock(cmdMutex);
      cmd = cmdQueue.front();
      cmdQueue.pop();
      return 0;
    }
    return -1;
  }
};
/**
 * @brief 系统中心控制类，负责整个系统的任务调度和节点管理
 */
class NodeManger{
private:
  // 节点指针
  /**
   * @brief 任务调度状态枚举
   */
  enum TaskScheduling {
    STOP = 0,           // 停止
    RUN = 1,            // 运行
    SchedulingError = 2 // 调度错误
  };
  TaskScheduling taskScheduling = RUN; // 任务调度状态
  // 命令解析线程
  std::thread cmdThread;
  std::thread nrtThread;

  // 命令销毁线程
  std::thread exit_cmd;

  // 命令对象指针容器
  std::pmr::monotonic_buffer_resource persistentNodePmr; // 实时节点内存资源
  std::pmr::vector<PersistentNode*> persistentNode;           // 实时节点容器
  ZrcsHardware::Controller *control;             // 硬件控制器指针
  //std::array<bool, 100> controlRegister = {};    // 控制寄存器
  //std::array<bool, 100> statusRegister = {};     // 状态寄存器
  RTProcess *rtProcess = nullptr;                // 实时进程指针
  OneShotNode *oneShotNode = nullptr;
  bool rtFlag = true; // 实时标志
  bool nrtFlag = true;
  // NodeCommunicaion<Motor> motorFeedback; // 电机反馈通信
public:
  // 命令队列
  CmdQueue *cmdQueue;

  /**
   * @brief 构造函数
   * @param rtProcess_ 实时进程指针
   */
  NodeManger(RTProcess *rtProcess_) : persistentNode(&persistentNodePmr),control(new ZrcsHardware::Controller()), cmdQueue(new CmdQueue()) {
    rtProcess = rtProcess_;
  }

  // 禁用拷贝构造函数
  NodeManger(const NodeManger &) = delete;
  // 禁用赋值操作符
  NodeManger &operator=(const NodeManger &) = delete;

  /**
   * @brief 析构函数，清理资源
   */
  ~NodeManger(void) 
  {

    delete control;
    delete cmdQueue;
    cmdThread.join();
    nrtThread.join();
  }
  

  /**
   * @brief 注册对象到系统中
   * @param cmd 命令字符串，包含类名和参数
   */
  void registerObject(std::string cmd, std::string &cmdName,std::string &cmdParam) 
  {
    if (cmd.npos != cmd.find_first_of(" --")) 
    {
      cmdName = cmd.substr(0, cmd.find_first_of(" --"));

      cmdParam = cmd.substr(cmd.find_first_of(" --"));
    } 
    else
    {
      cmdName = cmd;
    }
   
  }
  /**
   * @brief 运行系统主循环
   * 创建实时任务并启动任务调度器
   */
  void run() {
    cmdParsing();
    nrtThread = std::thread([this]() {
      while (true) {
        if (oneShotNode != nullptr) {
          oneShotNode->executeNrt();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });
    control->rtos_->rtos_task_create();
    // 将实时节点的实时函数放入实时线程
    control->rtos_->real_task([this]() {
      // control->receiveData();
     // controlRegister = rtProcess->shared_block_->registers.sysControl.load();
      switch (taskScheduling) 
      {
            case RUN:
              if (oneShotNode != nullptr)
               {
                  // 检查节点状态并执行相应操作
                  if (oneShotNode->GetOneShotStatus() == OneShotNodeStatus::COMPLETED) 
                  {
                    oneShotNode->popCmdArgs();
                    oneShotNode->SetOneShotStatus(OneShotNodeStatus::START);
                    // 节点已完成或失败，清理资源
                    oneShotNode = nullptr;
                  } 
                  else 
                  {
                    // 节点仍在运行，继续执行
                    oneShotNode->execute();
                  }
              }
              if (!persistentNode.empty()) 
              {   
                  for (auto &node : persistentNode)
                  {
                    node->execute();
                  }
              }
              break;
            case STOP:
              break;
            case SchedulingError:
              break;
            default:
              break;
            }
     // rtProcess->shared_block_->registers.sysStatus.store(statusRegister);
      // control->SendData();
    });
  }
  


  /**
   * @brief 命令解析函数
   * 启动命令解析线程，持续从命令队列中读取并处理命令
   */
  void cmdParsing() 
  {
    cmdThread = std::thread([this]() {
      while (rtFlag) 
      {
        Command cmd_;
        if (rtProcess->shared_block_->command_queue.pop(cmd_))
        {
            std::string cmd(cmd_.cmd);
            std::string cmdParam = {};
            std::string cmdName = {};
            registerObject(cmd, cmdName, cmdParam);
            if (NodeFactory<OneShotNode>::getInstance().exist(cmdName)) 
            {
                auto node =NodeFactory<OneShotNode>::getInstance().create(cmdName);
                if (node) 
                {
                    oneShotNode = node.release(); // 转移所有权
                    if (oneShotNode->GetOneShotStatus() ==OneShotNodeStatus::START)
                    {
                      oneShotNode->pushCmdArgs(cmdParam);
                      oneShotNode->SetOneShotStatus(OneShotNodeStatus::INIT);
                    } 
                    else 
                    {
                      std::cout << oneShotNode->getNodeNAME() << "状态错误"<< std::endl;
                      delete oneShotNode; // 如果状态错误，则清理
                      oneShotNode = nullptr;
                    }
                }
            } 
            else if (NodeFactory<PersistentNode>::getInstance().exist(cmdName)) 
            {
                auto node =NodeFactory<PersistentNode>::getInstance().create(cmdName);
                if (node) 
                {
                  // 处理持久节点
                  persistentNode.push_back(node.release());
                }
            }
            else 
            {
                std::cout << "指令不存在" << std::endl;
            }
          
         
      }
      
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });
  }
};
} // namespace zrcsSystem
#endif
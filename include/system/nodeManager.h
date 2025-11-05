/**
 * @copyright 版权所有(c)2024 Glroad公司
 * @filename: centre.h
 * @brief: 系统中心控制类，负责任务调度和节点管理
 * @author: zhangyongjing@oetsky.com
 * @createdate: 2024-01-04
 */
#ifndef NODEMANAGER_H_
#define NODEMANAGER_H_
#include "basenodeInterface.h"
#include "cmdline.h"
#include "command/Cmdhead.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include "controller/rtos/linux.h"
#include "nodeCommunication.h"
#include "nodeFactory.h"
#include <array>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>

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
    INIT,
    STOP,           // 停止
    RUN,            // 运行
    ERROR  // 调度错误
  };
  TaskScheduling taskScheduling =INIT; // 任务调度状态
  // 命令解析线程
  std::thread cmdThread;
  std::thread nrtThread;

  // 命令销毁线程
  std::thread exit_cmd;

  // 命令对象指针容器
  std::pmr::monotonic_buffer_resource outputPlcNodePmr; // 实时节点内存资源
  std::pmr::monotonic_buffer_resource inputPlcNodePmr; // 实时节点内存资源
  std::pmr::monotonic_buffer_resource rtCmdNodePmr; // 实时节点内存资源

  std::pmr::vector<OutputPlcNode*> outputPlcNode;       // 实时节点容器
  std::pmr::vector<InputPlcNode*> inputPlcNode;
  std::pmr::vector<RtCmdNode*> rtCmdNode;
  ZrcsHardware::Controller *control;             // 硬件控制器指针
  RTProcess *rtProcess = nullptr;                // 实时进程指针
  CmdNode*   cmdNode = nullptr;                     // 一次性命令节点指针
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
  NodeManger(RTProcess *rtProcess_) : outputPlcNode(&outputPlcNodePmr),inputPlcNode(&inputPlcNodePmr),rtCmdNode(&rtCmdNodePmr),control(new ZrcsHardware::Controller()), cmdQueue(new CmdQueue()) {
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
  void run() 
  {
    control->rtos_->rtos_task_create();
    // 将实时节点的实时函数放入实时线程
    control->rtos_->real_task([this]() 
    {
      control->receiveData();
      switch (taskScheduling) 
      {     Command cmd_ ;   
            case INIT:
                if (rtProcess->shared_block_->commandQueue.pop(cmd_))
                {
                   std::string_view cmdName(cmd_.cmd);
                   cmdNode =NodeFactory::getInstance().getNodePtr(cmdName).get();
                   cmdNode->registered(control,rtProcess);
                   taskScheduling= RUN;
                }               
              break; 
            case RUN:                              
                  if (cmdNode != nullptr)
                  { 
                    if (cmdNode->getCmdStatus() == CmdStatus::COMPLETED) 
                    {
                      cmdNode->setCmdStatus(CmdStatus::INIT);
                      taskScheduling= INIT;
                      // 节点已完成或失败，清理资源
                      cmdNode = nullptr;
                    } 
                    else if(cmdNode->getCmdStatus() == CmdStatus::FAILED)
                    {
                      // 节点仍在运行，继续执行
                      cmdNode = nullptr;
                      taskScheduling= INIT;
                    }
                    else 
                    {
                      // 节点仍在运行，继续执行
                      cmdNode->execute();
                    }                  
                  }
                break;
            case STOP:
              break;
            case ERROR:
              break;
            default:
              break;
      }
     control->SendData();
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
        if (rtProcess->shared_block_->commandQueue.pop(cmd_))
        {
            std::string cmd(cmd_.cmd);
            std::string cmdParam = {};
            std::string cmdName = {};
            registerObject(cmd, cmdName, cmdParam);
            // if (NodeFactory<CmdNode>::getInstance().exist(cmdName)) 
            // {
            //     auto node =NodeFactory<CmdNode>::getInstance().getNodePtr(cmdName);
            //     if (node) 
            //     {
            //         cmdNode = node.get(); 
            //         if (cmdNode->getCmdStatus() ==CmdStatus::START)
            //         {
            //           cmdNode->registered(control,rtProcess);
            //           cmdNode->pushCmdArgs(cmdParam);
            //           cmdNode->setCmdStatus(CmdStatus::INIT);
            //         } 
            //         else 
            //         {
            //           std::cout << cmdNode->getNodeNAME() << "状态错误"<< std::endl;
            //           cmdNode = nullptr;
            //         }
            //     }
            // }
            // else if (NodeFactory<RtCmdNode>::getInstance().exist(cmdName)) 
            // {
            //     auto node =NodeFactory<RtCmdNode>::getInstance().getNodePtr(cmdName);
            //     if (node) 
            //     {
            //       node->registered(control,rtProcess);
            //       rtCmdNode.push_back(node.get());
            //     }
            
            // }
            // else if (NodeFactory<InputPlcNode>::getInstance().exist(cmdName)) 
            // {
            //       auto node =NodeFactory<InputPlcNode>::getInstance().getNodePtr(cmdName);
            //       if (node) 
            //       {
            //         node->registered(control,rtProcess);
            //         inputPlcNode.push_back(node.get());
            //       }
            // }
            // else if (NodeFactory<OutputPlcNode>::getInstance().exist(cmdName)) 
            // {
            //     auto node =NodeFactory<OutputPlcNode>::getInstance().getNodePtr(cmdName);
            //     if (node) 
            //     {
            //       node->registered(control,rtProcess);
            //       outputPlcNode.push_back(node.get());
            //     }
            // }
            // else if (cmdName=="Stop") 
            // {
               
            // }
            // else if (cmdName=="Recover")
            // {
            
            // }
            // else if (cmdName=="RemoveNode") 
            // {
            //      if (!cmdParam.empty())
            //      { 
            //          const char* WHITESPACE = " \t\n\r\f\v";
            //          size_t first_char_pos = cmdParam.find_first_not_of(WHITESPACE);
            //         if (std::string::npos == first_char_pos) {
            //             // 如果字符串全是空格，则清空
            //             cmdParam.clear();
            //         } else {
            //             cmdParam.erase(0, first_char_pos);
            //         }
            //         auto node =NodeFactory<OutputPlcNode>::getInstance().getNodePtr(cmdParam);
            //         if (node) 
            //         {                          
            //                   auto it = std::find(outputPlcNode.begin(), outputPlcNode.end(), node.get());                                
            //                   if (it != outputPlcNode.end()) 
            //                   {
            //                       outputPlcNode.erase(it);
                                  
            //                   } 
            //                   else 
            //                   {
            //                       std::cout << "在vector中未找到该指针。" << std::endl;
            //                   }
            //         }                  
            //      }
            // }
            // else if (cmdName=="RemoveCmd") 
            // {
            //        cmdNode = nullptr;
            // }
            // else
            // {
            //     std::cout << "指令不存在" << std::endl;
            // }
          
         
      }      
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    });
  }
};
} // namespace zrcsSystem
#endif
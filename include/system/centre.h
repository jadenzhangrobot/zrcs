/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:centre.h
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-04
 */
#ifndef CENTRE_H_
#define CENTRE_H_
#include "basenode.h"
#include "classfactory.h"
#include "controller/controller_interface.h"
#include "controller/rtos/linux.h"
#include <algorithm>
#include <boost/bind/placeholders.hpp>
#include <endian.h>
#include <functional>
#include <iostream>
#include <memory_resource>
#include <mutex>
#include <queue>
#include <spdlog/spdlog.h>
#include <thread>
#include <vector>
#include "zmq.h"
#include "controller/rtos/preempt_rt.h"
#include "controller/rtos/xenomai.h"
#include "controller/motor/rawsocketbus.h"
#include "controller/motor/ethercat/EthercatMotor.h"

namespace zrcs_system {

class centre {
private:
  //节点指针
  Basenode *Bnode = nullptr;
  //解析指令的线程
  std::thread cmd_thread;
  //终端字符串接受线程
  std::thread terminal;
  //状态发布线程
  std::thread pubstatus;
  //指令销毁线程
  std::thread exit_cmd;
  std::thread zmq_thread;

  //将要指令对象指针容器
  std::pmr::monotonic_buffer_resource rtpmr;
  std::pmr::vector<Basenode *> rtnodeptr_vector;
  //执行完成后的指令对象的指针容器
  std::pmr::monotonic_buffer_resource exit_rtpmr;
  std::pmr::vector<Basenode *> exit_rtnodeptr_vector;

  bool terminal_flag = true;
  bool cmd_thread_flag = true;
  bool pubstatus_flag = true;
  bool exitcmd_flag = true;
  bool zmq_flag=true;
 
  //创建一个ros节点
   Zmq_cmd zmq_cmd; 
  
  
public:
  //指令参数队列
  std::queue<std::string> *cmdParam;
  //指令队列
  std::queue<std::string> cmd_queue;
  std::mutex CmdQueueMutex;
  std::mutex ZmqQueueMute;
  // controller对象指针
  controller::Controller *ec_control;
 
 // actionlib::SimpleActionServer<zrcsbt::zrcs_clientAction> Server;
 
  centre():rtnodeptr_vector(&rtpmr), exit_rtnodeptr_vector(&exit_rtpmr),
          ec_control(new controller::Controller() ),cmdParam(new std::queue<std::string>())        
  {
  }
  centre(const centre &) = delete;
  centre &operator=(const centre &) = delete;
  ~centre(void) {
    delete  ec_control;
    delete  cmdParam;
    terminal_flag = false;
    cmd_thread_flag = false;
    pubstatus_flag = false;
    exitcmd_flag = false;
    zmq_flag=false;
    
    cmd_thread.join();
    terminal.join();
    pubstatus.join();
    exit_cmd.join();
    zmq_thread.join();
  }

  //单例模式 返回一个
  static centre &getInstance(void) {
    static centre c_t;
    return c_t;
  }
 template<int JointNum,class motor,class transceive,class osal>
  void registerController() {
    for(int i=0;i<JointNum;i++)
    {
        std::unique_ptr<controller::Motor> cm((controller::Motor*)(new motor(i)));
        ec_control->motors.push_back(std::move(cm));
    }
     //发送接受函数，为了兼容总线协议
     ec_control->transceiver.reset((controller::Transceive*)(new transceive()));
      //使用linux操作系统
     ec_control->rtos_.reset((controller::Rtos*)(new osal()));
  }

  void registerObject(std::string cmd) {
    //对象名
    std::string class_name;
    // 指令参数字符串
    std::string cmd_param;

    if (cmd.npos != cmd.find_first_of(" --")) {
      class_name = cmd.substr(0, cmd.find_first_of(" --"));

      cmd_param = cmd.substr(cmd.find_first_of(" --"));

      cmdParam->push(cmd_param);
    } else {
      class_name = cmd;
    }
    if (!classfactory::getInstance().cmd_exist(class_name)) {
      std::cout<<"cmd不存在"<<std::endl;
    } else {
      Basenode *bn =
          (Basenode *)classfactory::getInstance().getclassbyname(class_name);
          //
           bn->registered(ec_control,cmdParam);
          //
           
      //把节点状态切换到init状态
      if (bn->getTaskState() == Basenode::IDLE) {
        bn->init();
      }
      if (bn->getTaskState() == Basenode::FAILURE) {
        spdlog::error(class_name + "init error");
      } else {
        rtnodeptr_vector.push_back(bn);
      }
    }
  }
  void init() {
    // 通过zmq获取命令字符串
    zmq_thread=std::thread([this](){
               while (zmq_flag) 
               {
                  //frame_test frame;          
                  char buffer[128]={};
               // std::string buffer;
               std::cout<<"----------------------------------"<<std::endl;
                  int ret= zmq_cmd.sub(buffer); 
                     printf("%s\n",buffer);        
                  if(ret<=0)
                  {
                     std::cout<<strerror(errno)<<std::endl;
                  }
                  else
                  {
                        std::string cmd(buffer);
                        if (cmd!="MotionCtr") 
                        {
                             std::cout<<"receive data---  "<<cmd<<std::endl;
                             //LOGGER_INFO("receive data");
                             ZmqQueueMute.lock();
                             cmd_queue.push(cmd);
                             ZmqQueueMute.unlock();
                        }
                        
                  }

                  
                  std::this_thread::sleep_for(std::chrono::milliseconds(100));   
               
               }

            });

    //终端获取指令字符串
    terminal = std::thread([this]() {
      while (terminal_flag) {
        //指令字符串
        std::string cmd;
        std::getline(std::cin, cmd);
        CmdQueueMutex.lock();
        cmd_queue.push(cmd);
        CmdQueueMutex.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });
    

    //从指令队列里面获取指令字符串
    cmd_thread = std::thread([this]() {
      while (cmd_thread_flag) {
        if (!cmd_queue.empty()) {
          std::string cmd_param = cmd_queue.front();
          if (cmd_param == "Stop") {
            if (Bnode != nullptr) {
              Bnode->stop();
            }
          } else if (cmd_param == "Start") {
            if (Bnode != nullptr) {
              Bnode->start();
            }
          } else if (cmd_param == "Recover") {
            if (Bnode != nullptr) {
              Bnode->recover();
            }
          } else {
            this->registerObject(cmd_param);
          }
          cmd_queue.pop();
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    //把指令任务销毁掉
    exit_cmd = std::thread([this]() {
      while (exitcmd_flag) {
        if (!exit_rtnodeptr_vector.empty()) {
          exit_rtnodeptr_vector.front()->exit();
          std::this_thread::sleep_for( std::chrono::milliseconds(1000));        
          delete exit_rtnodeptr_vector.front();

          exit_rtnodeptr_vector.erase(exit_rtnodeptr_vector.begin());
        }
      }
    });

    ec_control->transceiver->init();

    //创建一个实时任务
    ec_control->rtos_->rtos_task_create();
    //把实时节点里面的实时函数放到实时线程中运行
    ec_control->rtos_->real_task([&]() {
      ec_control->transceiver->receive();
      if (!rtnodeptr_vector.empty()) {
        Bnode = rtnodeptr_vector.front();
        if (Bnode != nullptr) {
          if (Bnode->getTaskState() == Basenode::RUNNING) {
            Bnode->excute_rt();
          } else if (Bnode->getTaskState() == Basenode::SUCCESS) {
            rtnodeptr_vector.erase(rtnodeptr_vector.begin());
            exit_rtnodeptr_vector.push_back(Bnode);
            //检查实时节点的状态，如何状态为running执行节点下的实时函数
          } else if (Bnode->getTaskState() == Basenode::FAILURE) {
            rtnodeptr_vector.erase(rtnodeptr_vector.begin());
            exit_rtnodeptr_vector.push_back(Bnode);
          }
        }
      }
      ec_control->transceiver->send();
    });
  }
};
} 
#endif
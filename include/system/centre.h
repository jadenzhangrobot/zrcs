/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 13:41:56
 * @LastEditTime: 2023-08-09 09:04:06
 * @Description: 生成一个centre类，包含所有类，对外提供一个单例
 *
 */
#ifndef CENTRE_H_
#define CENTRE_H_
#include "basenode.h"
#include "classfactory.h"
#include "cmdline.h"
#include "controller/controller_interface.h"
#include "controller/rtos/linux.h"
#include "ros/init.h"
#include "zmq.h"
#include <endian.h>
#include <queue>
#include <ros/ros.h>
namespace zrcs_system {
class centre {
private:
  std::thread zmq_thread;
  //终端字符串接受线程
  std::thread th_terminal;
  //行为树线程
  std::thread th_bttree;
  //接受行为树指令线程
  std::thread th_pubstatus;
  //指令对象指针队列
  std::queue<Basenode *> Basenode_queue;
  Basenode *Bnode = nullptr;
  bool flag = true;
  bool bttree_flag = true;
  bool zmq_flag = true;
  bool Init_flag = true;
  bool stop_flag = false;

public:
  //指令参数队列
  std::queue<std::string> nrt_cmdParam;
  //指令队列
  std::queue<std::string> cmd_queue;
  // controller对象指针
  controller::Controller *ec_control;
  //实时线程的初始状态为0
  // 0--表示初始状态
  // 1--表示执行指令中
  // 2--表示指令执行完毕
  // 3--表示指令执行失败
  Zmq_cmd zmq_cmd;
  centre(void) {}

  ~centre(void) {
    flag = false;
    bttree_flag = false;
    zmq_flag = false;
    Init_flag = false;
    zmq_thread.join();
    th_terminal.join();
    th_bttree.join();
    th_pubstatus.join();
  }

  //单例模式 返回一个
  static centre &getInstance(void) {
    static centre c_t;
    return c_t;
  }

  void registerController(std::unique_ptr<controller::Controller> ec_contol_) {

    ec_control = ec_contol_.release();
  }

  void registerObject(std::string cmd){
    //对象名
    std::string class_name;
    // 指令参数字符串
    std::string cmd_param;


    if (cmd.npos != cmd.find_first_of(" --")) {
      class_name = cmd.substr(0, cmd.find_first_of(" --"));

      cmd_param = cmd.substr(cmd.find_first_of(" --"));

      nrt_cmdParam.push(cmd_param);
    } else {
      class_name = cmd;
    }
    if (!classfactory::getInstance().cmd_exist(class_name)) {

    } else {
      Basenode *bf =(Basenode *)classfactory::getInstance().getclassbyname(class_name);
      //把节点状态切换到init状态
     
      // if (bf->config())
      // {
      //   if (bf->init())
      //    {
      //     if (bf->run()) 
      //     {
            Basenode_queue.push(bf);
      //     }
      //   }
      // }
    
    }
  }

  void init() {

    zmq_thread = std::thread([this]() {
      while (zmq_flag) {

        char buffer[100] = {};

        int ret = zmq_cmd.sub(buffer);
        if (ret <= 0) {
          std::cout << strerror(errno) << std::endl;
        } else {
          std::string cmd(buffer);
          cmd_queue.push(cmd);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    });

    //终端获取指令字符串
    th_terminal = std::thread([this]() {
      while (flag) {
        //指令字符串
        std::string cmd;
        std::getline(std::cin, cmd);
      //  std::string cmd1;
      //     std::string cmd2;
      //  cmd1="JogabsJ --motor=0 --position=1.5708";
      
      //  cmd2="JogabsJ --motor=0 --position=0"; 


       
        
      //    cmd_queue.push(cmd1);
      //    cmd_queue.push(cmd2);
         cmd_queue.push(cmd);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
       
      }
    });
    //从行为树获取指令字符串
    th_bttree = std::thread([this]() {
      while (bttree_flag) {
        if (!cmd_queue.empty()) {
          std::string cmd_param = cmd_queue.front();
          if (cmd_param == "Stop") {
            if (Bnode != nullptr) {
              Bnode->stop();
            }
          }
          if (cmd_param=="Start"){
              if (Bnode!=nullptr){
               Bnode->start();
               std::cout<<Bnode->getTaskState()<<std::endl; 
              }
          }
          else {
          this->registerObject(cmd_param);
          }          
          cmd_queue.pop();
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
  );

//  th_pubstatus = std::thread([this]() {

//    });
   
  ec_control->transceiver->init();
  
  //创建一个实时任务
  ec_control->rtos_->rtos_task_create();
  //把实时节点里面的实时函数放到实时线程中运行
  ec_control->rtos_->real_task([&]() {
  ec_control->transceiver->receive();

    if (!Basenode_queue.empty()) {
      Bnode = Basenode_queue.front();
     // std::cout<<Bnode->getTaskState()<<std::endl;   
       if(Bnode->config())
       {
       if (Bnode->init())
          {
           if (Bnode->run()) 
          {
          }
          }
       }
      //检查实时节点的状态，如何状态为running执行节点下的实时函数
      if (Bnode->getTaskState() == Basenode::RUNNING){
        Bnode->excute_rt();
      }
      //检查节点的状态如何节点执行成功，就从队列里面移除节点指针
      if (Bnode->getTaskState() == Basenode::SUCCESS){
          Basenode_queue.pop();          
      }
    }
     ec_control->transceiver->send();
  });
 
}

};
} // namespace zrcs_system
#endif
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
#include "controller/controller_interface.h"
#include "controller/rtos/linux.h"
#include "ros/init.h"
#include "zmq.h"
#include <endian.h>
#include <iostream>
#include <ostream>
#include <ros/ros.h>
#include <thread>
#include "spd_log.h"

namespace zrcs_system {
class centre {
private:
  std::thread cmd_thread;
  //终端字符串接受线程
  std::thread terminal;
  //指令注册线程
  std::thread pubstatus;


  //指令对象指针队列
  std::queue<Basenode*> Basenode_queue;
  Basenode *Bnode = nullptr;


  
  bool terminal_flag = true;
  bool cmd_thread_flag = true;
  bool pubstatus_flag=true;
public:
  //指令参数队列
  std::queue<std::string> nrt_cmdParam;
  //指令队列
  std::queue<std::string> cmd_queue;
  // controller对象指针
  controller::Controller *ec_control;
  centre(void){}

  ~centre(void) {
    terminal_flag = false;
    cmd_thread_flag = false;
    pubstatus_flag = false;
    cmd_thread.join();
    terminal.join();
    pubstatus.join();
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
      spdlog::info("cmd not exit");
    } else {
      Basenode *bf =(Basenode *)classfactory::getInstance().getclassbyname(class_name);
      //把节点状态切换到init状态
              if (bf->getTaskState()==Basenode::IDLE) {
                  bf->init();
              }
             if(bf->getTaskState()==Basenode::FAILURE)
               {
                     
               }             
             else
              {
                  Basenode_queue.push(bf);
              }         
        }
      
    
    }
  

  void init() {

    //终端获取指令字符串
    terminal = std::thread([this]() {
      while (terminal_flag) {
        //指令字符串
        std::string cmd;
       std::getline(std::cin, cmd);
        cmd_queue.push(cmd);
      // static int cmd_flag=0;
      // if (cmd_flag==0) {
      // std::string cmd1;
      //  cmd1="MoveSine";
      //  cmd_queue.push(cmd1);
      //  cmd_flag=1;
      // }
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
          }
          else if (cmd_param=="Start"){
              if (Bnode!=nullptr){
               Bnode->start();
               std::cout<<Bnode->getTaskState()<<std::endl; 
              }
          }
          else if (cmd_param=="Recover") {
              if (Bnode!=nullptr){
               Bnode->recover();
               std::cout<<Bnode->getTaskState()<<std::endl; 
              }
          }
          else {
          this->registerObject(cmd_param);
          }          
          cmd_queue.pop();
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
  );

  pubstatus = std::thread([this]() {
    

     std::this_thread::sleep_for(std::chrono::milliseconds(100));
   });
   

  

  ec_control->transceiver->init();
  
  //创建一个实时任务
  ec_control->rtos_->rtos_task_create();
  //把实时节点里面的实时函数放到实时线程中运行
  ec_control->rtos_->real_task([&]()
  {
      ec_control->transceiver->receive();
       if (!Basenode_queue.empty())
       {    
          Bnode=Basenode_queue.front();
          if (Bnode!=nullptr) {
                    if (Bnode->getTaskState() == Basenode::RUNNING)
                   {
                        Bnode->excute_rt();
                   }                 
                    else if (Bnode->getTaskState()==Basenode::SUCCESS)
                   {
                       Basenode_queue.pop();   
                    //检查实时节点的状态，如何状态为running执行节点下的实时函数           
                   }
                    else if (Bnode->getTaskState()==Basenode::FAILURE)
                   {
                       Basenode_queue.pop();  
                   }
            }
      }
      
     ec_control->transceiver->send();
  });
 
}

};
} // namespace zrcs_system
#endif
/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:centre.h
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-04
 */
#ifndef CENTRE_H_
#define CENTRE_H_
#include <any>
#include <mutex>
#include <thread>
#include <vector>
#include "basenodeInterface.h"
#include "classfactory.h"
#include "controller/Controller.h"
#include "controller/ControllerInterface.h"
#include "controller/rtos/linux.h"
#include "nodeCommunication.h"



#include "nodeCommunication.h"
#include "dataType.h"
#include "rt/rt_process.h"
#include "rt/rt_process.h"
namespace zrcsSystem {
class CmdQueue{
   
     std::mutex cmdMutex;
     std::queue<std::string> cmdQueue;
     public:
     CmdQueue()
     {

     }
      void writeCmd(std::string& cmd)
      {   
           std::lock_guard<std::mutex> lock(cmdMutex);
           cmdQueue.push(cmd);
      }
      int cmdRead(std::string& cmd)
      { 
            if (!cmdQueue.empty())
            {
                std::lock_guard<std::mutex> lock(cmdMutex);
                cmd = cmdQueue.front();
                cmdQueue.pop();
                return 0;
            }
            return -1;
      }

};
class Centre {
private:
  //node pointer
  enum TaskScheduling
  {
    STOP = 0,
    RUN = 1,
    SchedulingError = 2
  };
  TaskScheduling  taskScheduling=RUN;
  //command parsing thread
  std::thread cmdThread;

  //command destruction thread
  std::thread exit_cmd;

  //command object pointer container
  std::pmr::monotonic_buffer_resource rtCmdPmr;
  std::pmr::monotonic_buffer_resource rtNodePmr;
  std::pmr::vector<Basenode*> rtCmd;
  std::pmr::vector<Basenode*> rtNode; 
  ZrcsHardware::Controller *control;


  Basenode *Bnode = nullptr;
  bool rtFlag=true;
 // NodeCommunicaion<Motor> motorFeedback; 
public:
  //command queue
  CmdQueue* cmdQueue;
  Centre():rtCmd(&rtCmdPmr), rtNode(&rtNodePmr), control(new ZrcsHardware::Controller()),cmdQueue(new CmdQueue())
  {
     
  }
  Centre(const Centre &) = delete;
  Centre &operator=(const Centre &) = delete;
  ~Centre(void) {
   
    delete  control;
    delete cmdQueue;
    cmdThread.join();
  }
  void registerObject(std::string cmd)
   {
    //object name
    std::string class_name;
    // command parameter string
    std::string cmd_param;

    if (cmd.npos != cmd.find_first_of(" --")) 
    {
      class_name = cmd.substr(0, cmd.find_first_of(" --"));

      cmd_param = cmd.substr(cmd.find_first_of(" --")); 
    } 
    else
    {
      class_name = cmd;
    }
    if(!classfactory::getInstance().cmdExist(class_name))
    {
       std::cout<<"cmd not exist"<<std::endl;
    }
    else 
     { 
        if(classfactory::getInstance().getClassByName(class_name).type()==typeid(Basenode*))
        {
                  Basenode *bn =std::any_cast<Basenode*>(classfactory::getInstance().getClassByName(class_name));                   
                  //switch node state to init state                
                  if (bn->GetTaskState()==Basenode::IDLE) 
                  {
                      bn->registered(control); 
                      bn->SetTaskState(Basenode::INIT);
                  }
                  if (!cmd_param.empty()) 
                  {
                    bn->PushCmdArgs(cmd_param);
                  }                  
                  bn->config();
                  rtCmd.push_back(bn);
        }
        if (classfactory::getInstance().getClassByName(class_name).type()==typeid(CreateNode)) 
        { 
               CreateNode cn =std::any_cast<CreateNode>(classfactory::getInstance().getClassByName(class_name)); 
               Basenode* bn= (*cn)();
               if (bn->GetTaskState()==Basenode::IDLE) 
                  {
                      bn->registered(control); 
                      bn->SetTaskState(Basenode::INIT);
                  }
                  if (!cmd_param.empty()) 
                  {
                    bn->PushCmdArgs(cmd_param);
                  }                  
                  bn->config();
                  rtNode.push_back(bn);        
        }           
     }
  }
  void run()
  {
    //create a real-time task
    control->rtos_->rtos_task_create();
    //put real-time function from real-time node into real-time thread
    control->rtos_->real_task([this]() {   
    //control->receiveData();
      
      switch (taskScheduling) 
      {

        if (!rtNode.empty())
        {
           for (int i=0; i<rtNode.size(); i++)
           {
                  rtNode[i]->excuteRt();
                  if (rtNode[i]->GetTaskState()==Basenode::FAILURE) 
                  {
                      taskScheduling = TaskScheduling::SchedulingError;
                  }
           }
        }
        case RUN:
        if (!rtCmd.empty()) 
        {
          Bnode = rtCmd.front();
          if (Bnode != nullptr) 
          {
                if ((Bnode->GetTaskState() == Basenode::INIT)||Bnode->GetTaskState() == Basenode::EXIT)
                {
                      Bnode->init();
                }
                else if(Bnode->GetTaskState() == Basenode::RUNNING) 
                  {   
                      Bnode->excuteRt();
                        
                  } 
                else if (Bnode->GetTaskState() == Basenode::SUCCESS) 
                  {
                    
                    Bnode->exit();
                    rtCmd.erase(rtCmd.begin()); 
                          
                  } 
                else if (Bnode->GetTaskState() == Basenode::FAILURE) 
                  {
                    rtCmd.erase(rtCmd.begin());
                    taskScheduling = TaskScheduling::SchedulingError;
                  }
                else 
                  {
                      exit(1);
                  }
           }
          else {             
               rtCmd.erase(rtCmd.begin());
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
     //  control->SendData(); 
    });
  }
};
} 
#endif
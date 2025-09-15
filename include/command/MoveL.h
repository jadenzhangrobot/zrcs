/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:49:54
 * @LastEditTime: 2023-06-16 09:54:19
 * @Description: 直线运动指令
 */
#ifndef MOVEL_H
#define MOVEL_H

#include "system/basenodeInterface.h"
#include "model/modeinterface.h"
#include <array>
#include <ruckig/ruckig.hpp>
#include <string>
#include <vector>
#include "system/nodeFactory.h"
#include <cmath>

using namespace ruckig;
class MoveL : public zrcsSystem::RtCmdNode 
{
public:
  MoveL()
  {
   
  }
  
  void init() override
  {  
    MotionParam param;
    param.model = 0;
    param.x = 0;
    param.y = 0;
    param.z = 0;
    param.a = 0;
    param.b = 0;
    param.c = 0;
    param.velocity = 0;
    rtProcess->shared_block_->motionParamQueue.pop(param);
  }

   void run(void) override
  {
      
  }
  void exit(void) override 
  { 
     
  }
};
REGISTERRTCMD(MoveL)

#endif
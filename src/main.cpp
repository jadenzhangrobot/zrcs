/**
 * @file main.cpp
 * @author zhangyongjing (6499894200@qq.com)
 * @brief 
 * @version 1.0
 * @date 2024-11-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "command/Cmdhead.h"
#include "controller/ControllerInterface.h"
#include "system/centre.h"
#include "system/zrcs.h"

int main(int argc, char **argv) 
{
   try {
      zrcsSystem::Zrcs zs;
      zs.run();
      while(1)
      {
        sleep(1);
      }
  return 0;
    } catch (const std::runtime_error& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl; // 也可以使用 std::cerr 输
    }
}

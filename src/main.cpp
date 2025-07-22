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

#include "controller/ControllerInterface.h"
#include "system/nodeManager.h"
#include "system/zrcs.h"
#include <unistd.h>
#include <vector>

int main(int argc, char **argv) 
{
    try 
    {
      zrcsSystem::Zrcs zs;
      zs.run();
      while(true)
      {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    } 
    catch (const std::runtime_error& e) 
    {
        std::cerr << "Exception caught: " << e.what() << std::endl; 
    }
    return 0;
}

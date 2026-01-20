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

#include "common/sharedMemory/sharedData.h"
#include "controller/ControllerInterface.h"
#include "system/nodeManager.h"
#include <unistd.h>
#include <vector>
#include "command/Cmdhead.h"
int main(int argc, char **argv) 
{
 
  #ifdef __linux__
  if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        // 打印错误信息，这非常重要！
        fprintf(stderr, "Error: mlockall failed: %s\n", strerror(errno));        
        // 在生产环境中，通常应该在这里直接退出，因为实时性无法保证
        // exit(EXIT_FAILURE); 
    }   
    printf("Memory successfully locked.\n");
#endif
    try 
    {
      zrcsSystem::NodeManger nodeManger;
      nodeManger.run();
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

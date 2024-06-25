/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:main.cpp
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-08
 */
#include "command/Cmdhead.h"
#include "controller/rtos/xenomai.h"
#include "system/centre.h"
#include <unistd.h>
#include <vector>
int main(int argc, char **argv) {
  std::vector<int> a{2,3,5};
  // int b=3;
  // int d=4;
  // int c=5;
  // a.push_back(b);
  // a.push_back(d);
  // a.push_back(c);


  zrcs_system::centre &ct = zrcs_system::centre::getInstance();
  ct.registerController<1, controller::EthercatMotor,controller::EthercatTransceive, controller::xenomai>();
  ct.init();
  while(1)
  {
    sleep(1);
  }
  return 0;
}

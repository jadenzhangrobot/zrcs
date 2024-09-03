/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:main.cpp
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-08
 */
#include "command/Cmdhead.h"
#include "controller/controller_interface.h"
#include "controller/motor/ethercat/EthercatIo.h"
#include "controller/rtos/xenomai.h"
#include "system/centre.h"
#include <unistd.h>
#include "system/OpcuaServer.h"
#include "system/zrcs.h"

int main(int argc, char **argv) {

   zrcs_system::Zrcs zs;
   zs.ct->registerController<11, controller::EthercatMotor,controller::EthercatTransceive, controller::xenomai,controller::EthercatIo>();
   zs.init();
   zs.run();
  while(1)
  {
    sleep(1);
  }
  return 0;
}

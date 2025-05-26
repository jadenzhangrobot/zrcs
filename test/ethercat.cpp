
#include "controller/ethercat/EthercatMaster.h"
#include "controller/rtos/xenomai.h"
int main()
{
    ZrcsHardware::EthercatMaster ecat=ZrcsHardware::EthercatMaster();
    ZrcsHardware::xenomai xeno;
    xeno.rtos_task_create();
    //把实时节点里面的实时函数放到实时线程中运行
    xeno.real_task([&]()
    {
        ecat.receive();
        ecat.send();

    });

    while (true) {
      sleep(1);
    }
  return 0;
}

/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:main.cpp
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-08
 */
#include "command/Cmdhead.h"
#include "ros/rate.h"
#include "system/centre.h"
#include <ros/ros.h>
#include <unistd.h>
int main(int argc, char **argv) {
//    printf ("Connecting to hello world server…\n");
//     void *context = zmq_ctx_new ();
//     void *requester = zmq_socket (context, ZMQ_REQ);
//     zmq_connect (requester, "tcp://10.16.11.77:5555");

//     int request_nbr;
//     for (request_nbr = 0; request_nbr != 10; request_nbr++) {
//         char buffer [10];
//         printf ("Sending Hello %d…\n", request_nbr);
//         zmq_send (requester, "Hello", 5, 0);
//         zmq_recv (requester, buffer, 10, 0);
//         printf ("Received World %d\n", request_nbr);
//     }
//     zmq_close (requester);
//     zmq_ctx_destroy (context);
//     return 0;
#ifdef Ros
  ros::init(argc, argv, "motion_control");
#endif
  zrcs_system::centre &ct = zrcs_system::centre::getInstance();
  ct.registerController<6, controller::zmotionmotor,controller::ZmotionTransceive, controller::Nativelinux>();
  ct.init();
#ifdef Ros
  ros::spin();
#else
  pause();
#endif
  return 0;
}

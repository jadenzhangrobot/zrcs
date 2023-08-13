/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-04-28 14:50:21
 * @LastEditTime: 2023-06-20 16:34:59
 * @Description: 
 * 
 */
#ifndef GLRBUS_H
#define GLRBUS_H
#include <cstdint>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <netpacket/packet.h>
#include <pthread.h>
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include "controller/controller_interface.h"
#include <atomic>
#include <cstddef>
#define joints 1
namespace controller
 {
   template <typename T>
   inline void set_bit(T& data, int bit_position) 
    {
    // 将数字 1 左移相应的位数，得到一个只有特定比特位为1的数
     T bit_mask = 1 << bit_position;

    // 将数据和位掩码进行按位或运算，将特定比特位设置为1
    data |= bit_mask;
   }
template <typename T>
inline void clear_bit(T& data, int bit_position) {
    // 将数字 1 左移相应的位数，得到一个只有特定比特位为0的数的补码
    T bit_mask = ~(1 << bit_position);

    // 将数据和位掩码进行按位与运算，将特定比特位设置为0
    data &= bit_mask;
}
template <typename T>
inline bool get_bit(T data, int bit_position) {
    // 将数字 1 左移相应的位数，得到一个只有特定比特位为1的数
    T bit_mask = 1 << bit_position;

    // 将数据和位掩码进行按位与运算，获取特定比特位的值
    unsigned char result = data & bit_mask;

    // 如果特定比特位的值为0，返回false；否则返回true
    return (result != 0);
}
//16bit computer--->network htons()
//16bit network--->computer ntohs()
//32bit computer--->network htonl()
//32bit network--->computer ntohl

class Glrbus
 {
     private:
       int sockhandle;
       struct ifreq ifr;
        struct ifreq ifr1;
       struct sockaddr_ll sa;
       struct timeval timeout;
       int  ifindex;
         struct contrl_data
         {
           const uint8_t  src[6]={0xD4,0x9E,0x6D,0x00,0x00,0xff};//本机地址
           const uint8_t  dst[6]={0xff,0xff,0xff,0xff,0xff,0xff};//广播地址
           const uint8_t type[2]={0xff,0x01};
           uint16_t sequence;
           uint16_t delay_time;
           uint8_t rsv[4];
         };               
    public:
        bool rt_sendflag=0;
        inline static std::array<std::atomic<uint8_t>,206> mypacket_send;
        inline static std::array<std::atomic<uint8_t>,206> mypacket_receive;
        inline static std::array<std::atomic<uint8_t>,58+12+12*joints> mypacket_send_init;
        inline static std::array<std::atomic<uint8_t>,58+12+12*joints> mypacket_receive_init;
        inline static uint16_t sequence;
        ~Glrbus()
        {
          close(sockhandle);
        }

         Glrbus(void )
        {
                        
        }
        void init()
        {
          //const char * interface ="enP4p65s0";
                const char * interface ="eth1";
               // const char * interface ="lo";
               
                size_t if_name_len=strlen(interface);
                if (if_name_len<sizeof(ifr.ifr_name)) {
		             memcpy(ifr.ifr_name,interface,if_name_len);
	             	ifr.ifr_name[if_name_len]=0;
	
                  	}
                int ret = 0;

              sockhandle = socket( PF_PACKET, SOCK_RAW, htons(ETH_P_ALL) );
               if( sockhandle < 0 )
                {
                     //LOGGER_INFO("glrbus socket error");
                     printf("socket error : %s\n", strerror(errno));   
                  
                  
                }
              timeout.tv_sec = 0;
              timeout.tv_usec = 1; 
              setsockopt(sockhandle, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
              setsockopt(sockhandle, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
              int i = 1;
              setsockopt(sockhandle, SOL_SOCKET, SO_DONTROUTE, &i, sizeof(i));
              // strcpy(ifr.ifr_name, interface);
              if(ioctl(sockhandle, SIOCGIFINDEX, &ifr))
              {
                      printf("ioctl1 : %s\n", strerror(errno));   

              }
             

                 ifindex = ifr.ifr_ifindex;



              strcpy(ifr1.ifr_name, interface);
              
               ifr1.ifr_flags = 0;
              /* reset flags of NIC interface */
              if(ioctl(sockhandle, SIOCGIFFLAGS, &ifr1))
              {
                  printf("ioctl2: %s\n", strerror(errno)); 

              }
              /* set flags of NIC interface, here promiscuous and broadcast */
              ifr1.ifr_flags = ifr1.ifr_flags | IFF_PROMISC | IFF_BROADCAST;
              if(ioctl(sockhandle, SIOCSIFFLAGS, &ifr1))
              {
                  printf("ioctl3: %s\n", strerror(errno)); 
              }
                sa.sll_family=AF_PACKET ;
                sa.sll_protocol =htons(ETH_P_ALL);
                sa.sll_ifindex = ifindex;
              
                // Not sure why, this is required for receiving on some systems.
                ret=bind( sockhandle,(const struct sockaddr *)&sa, sizeof(sa));
                if( ret < 0 )
                 {
                        perror("bind");
                        //LOGGER_INFO("glrbus bind error");
                      //  LOGGER_INFO(strerror(errno));

                   }
                ret = setsockopt( sockhandle, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface));   
                if( ret < 0 )
                   {     
                                    
                       // LOGGER_INFO("glrbus setsockopt error");
                        //LOGGER_INFO(strerror(errno));
                   }
           

        }
        int bus_send()
        {
            if(rt_sendflag)
            {
               static struct sockaddr_ll socket_address;
               contrl_data cd;
               
              
              sequence++;
              cd.sequence=htons(sequence);
              mempcpy(mypacket_send.data(),&cd,sizeof(cd));     

            // Setup sending address for packet.
              
             socket_address.sll_ifindex = ifr.ifr_ifindex;
             socket_address.sll_halen = ETH_ALEN;
             memcpy( socket_address.sll_addr, mypacket_send.data()+6, 6 );
              
              static uint64_t count=0;
             int ret=sendto(sockhandle, mypacket_send.data(),mypacket_send.size(), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
                if(ret<0)
                {
                 //  LOGGER_INFO("glrbus send error");
                  // LOGGER_INFO(strerror(errno));
                   return -1;
               }

              
               printf("%ld\n",count);
               count++;
            } 

                     
             
              // for(int i=0;i<2000;i++)
              // {
               // static uint16_t tttt=0;
               
              //   if(tttt/10000){
              //                    unsigned char array[] = {
              //       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              //       0xff, 0xff, 0xf0, 0xff, 0xff, 0x02, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
              //       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, };
              //         struct sockaddr_ll socket_address;
              //          socket_address.sll_ifindex = ifr.ifr_ifindex;
              //         socket_address.sll_halen = ETH_ALEN;
              //      memcpy( socket_address.sll_addr, array+6, 6 );
              //    int ret=sendto(sockhandle,array,142, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
              //   if(ret<0)
              //   {
              //      LOGGER_INFO("glrbus send error");
              //      LOGGER_INFO(strerror(errno));
              //      return -1;
              //  }
              // }
              //  tttt++;
          return 1;        
        }

        
        int bus_send_init()
        {
            
            struct sockaddr_ll socket_address; 
           
            socket_address.sll_ifindex = ifr.ifr_ifindex;
            socket_address.sll_halen = ETH_ALEN;
            memcpy( socket_address.sll_addr, mypacket_send.data()+6, 6);
         
            int ret=sendto(sockhandle, mypacket_send_init.data(),mypacket_send_init.size(), 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
             
               if(ret<0)
               {
                   printf("Failed to open file: %s\n", strerror(errno));      
                  // LOGGER_INFO("glrbus send error");
                  // LOGGER_INFO(strerror(errno));
                  return -1;
               }
               else
               {
                 return ret;
               }           
        }

        int  bus_recv()
        {
            
             uint8_t buff[1500];
		       int rx = recv( sockhandle, buff, 1500, 0);
          
            //  if(buff[12]==0xff&&buff[13]==0x00)
            // {
            //    //std::cout<<"hhhh"<<std::endl;
            //   // if(buff[17]==1)
            //   // {
            //        rt_sendflag=1;
                
            //  // }
            //    memcpy(mypacket_receive_init.data(),buff,mypacket_receive_init.size());
               
            //   for(int i=0;i<28;i++)
            //   {
            //     printf("%02x ",mypacket_receive_init[i].load());          
            //   }
            //  printf("\n");
            // }
             
            //  if(buff[12]==0xff&&buff[13]==0x01)
            // {
            //   memcpy(mypacket_receive.data(),buff,mypacket_receive.size());
            //    uint16_t write;
            //    static uint16_t before;
            //    memcpy(&write,buff+14,2);
       
             
            //   before=ntohs(write);
              
            //   if(before==sequence)
            //   {
                   
                    
            //   }
            //   else {
             
            //          static uint32_t a=0;
            //          a++;
            //         std::cout<<a<<std::endl;

            //   }
             
            // }
                     
             return rx;
        }
        
        static Glrbus &getInstance()
        {
            static Glrbus motor_instance;
            return motor_instance;
        }     

 };
 
  class Glrrobot:public Motor
  {
    private:
        struct mac
        {
            const uint8_t  src[6]={0xD4,0x9E,0x6D,0x00,0x00,0xff};//本机地址 
            const uint8_t  dst[6]={0xff,0xff,0xff,0xff,0xff,0xff};//广播地址
                    
        };
         struct  init_slave_status
         {
            uint8_t fpga[4];
            uint8_t mcu[4];
            uint8_t board_status;
            uint8_t board_type;
            uint8_t rsv[2];
         };
         
        struct init_data
        {
            mac mac_addr;
            const uint8_t type[2]={0xff,0x00};
            uint16_t sequence_id;
            uint8_t joints_num;
            uint8_t joint_sts;
            uint8_t yjoint_sts;
            init_slave_status iss[3+joints];          
        };




         struct motor_contr
        {
             uint8_t cmd;
             uint8_t rsv[3];
             int32_t angle;
             int32_t vel_feedforward;
             int32_t tor_feedforward;
        };
        struct motor_status
        {
            uint8_t error[3];
            uint8_t board_status;
            uint16_t current2;
            uint16_t current;
            int32_t feedback_angle;
            int32_t feddback_angle2;
            int32_t angle_vel;
        };      
       struct contrl_data
       {
          mac mac_addr;
          const uint8_t type[2]={0xff,0x01};
          uint16_t sequence;
          uint16_t delay_time;
          uint8_t rsv[4];
          motor_contr mc;
          motor_status ms;
       };        
   
     public:
         int motor_id;
         Glrbus &glrbus=Glrbus::getInstance();
         
         Glrrobot(int id):motor_id(id)
         {
             

         };
         ~Glrrobot() 
         {

         };
          public:
          auto init()->int override
          {              
                init_data id;
                static uint16_t sequence=0;
                sequence++;
                id.sequence_id=htons(sequence);
                id.joint_sts=0;
                id.yjoint_sts=0;
                id.joints_num=1;

	            

              memcpy(glrbus.mypacket_send_init.data(),&id,sizeof(id)); 
              glrbus.bus_send_init();           
              return 1;
          }
          auto setTargetPos(double pos)->int override
           {
              size_t offset=offsetof(contrl_data,mc.angle)+16*motor_id;
              mempcpy(glrbus.mypacket_send.data()+offset,&pos,sizeof(pos));
              return 1;
           }
          auto actualPos()->double override
            {               
               double pos;             
               size_t offset=offsetof(contrl_data,ms.feddback_angle2)+16*motor_id;
               mempcpy(&pos,glrbus.mypacket_receive.data()+offset,sizeof(pos));
               return pos;
               
            }
          auto  disable()->int override
          {
                
                // contrl_data cd;
                // cd.mc[motor_id].cmd=pos;
                // mempcpy(glrbus.mypacket_send.data(), &cd , glrbus.mypacket_send.size());
                return 1;
          }
          
          auto  enable()->int override
          {
                
                  uint8_t enable=0x08;
                  size_t offset=offsetof(contrl_data,mc.cmd)+16*motor_id;
                  mempcpy(glrbus.mypacket_send.data()+offset,&enable,sizeof(enable));
                 return 1;
          }
         
         
          auto  mode(std::uint8_t md)->int override
          {
                uint8_t mode;
                switch (md) 
                {
                case 0:
                    mode=0x0c;
                    break;
                case 1:
                    mode=0x0a;
                    break;
                case 2:
                    mode=0x09;
                    break;
                default:                  
                    //LOGGER_INFO("mode error");
                    break;
                }
                  size_t offset=offsetof(contrl_data,mc.cmd)+16*motor_id;
                  mempcpy(glrbus.mypacket_send.data()+offset,&mode,sizeof(mode));
                  return 1;
          }

          auto  errorCode()->uint32_t override
          {
               uint32_t error;         
               size_t offset=offsetof(contrl_data,ms.error)+16*motor_id;
               mempcpy(&error,glrbus.mypacket_receive.data()+offset,3);
               return error;
          }
    };

  class GlrrobotTransceive:Transceive
  {
    private:
         Glrbus& glrbus=Glrbus::getInstance();
    public: 
        auto init(void)->int  override
        {

          glrbus.init();
          return 1;
        }
        auto send(void)->void override
        {        
             
          glrbus.bus_send();
        }
        auto  receive()->void override
        {
          glrbus.bus_recv();
        }
  };


 }
 
#endif

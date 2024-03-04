/**
 * @copyrightCopyright(c)2024Glroadcorporation
 * @filename:zmotion.h
 * @brief:
 * zhangyongjing@oetsky.com
 * @createdate:2024-01-05
 */
#ifndef ZMOTION_H
#define ZMOTION_H
#include "zmotion/zmotion.h"
#include "zmotion/zmcaux.h"
#include "controller/controller_interface.h"

namespace controller
{    
     class zmotion{
     public:
     ZMC_HANDLE handle = nullptr; //连接句柄
     char ip[13]="192.168.0.11";
     int m_nAxisList[2] = {4,5}; //轴列表
    
      zmotion(){
        //  int rtn = ZAux_OpenEth(ip, &handle); //连接控制器
        // //if(CheckError(rtn,"ZAux_OpenEth")) return; //检查函数返回
        // for(int iAxis = 4;iAxis<6;iAxis++)
        // {
        //     rtn = ZAux_Direct_SetAtype(handle,iAxis,21); //设置轴为振镜轴
        //     //if(CheckError(rtn,"ZAux_Direct_SetAtype")) return;
        //     rtn = ZAux_Direct_SetUnits(handle,iAxis,200); //设置轴脉冲当量
        //     // if(CheckError(rtn,"ZAux_Direct_SetUnits")) return;
        //     rtn = ZAux_Direct_SetMerge(handle,iAxis,1); //设置轴开启连续插补
        //     // if(CheckError(rtn,"ZAux_Direct_SetMerge")) return
        //     int  rtn = ZAux_Direct_SetSpeed(handle,m_nAxisList[0],1000); //设置运动速度 1000
        //     //if(CheckError(rtn,"ZAux_Direct_SetSpeed")) return;
        //     rtn = ZAux_Direct_SetAccel(handle,m_nAxisList[0],10000); //设置运动加速度 10000
        //    //if(CheckError(rtn,"ZAux_Direct_SetAccel")) return;
        //     rtn = ZAux_Direct_SetDecel(handle,m_nAxisList[0],10000); //设置运动减速度 1000

        //     // if(CheckError(rtn,"ZAux_Direct_SetDecel")) return;
        //     rtn = ZAux_Direct_SetSramp(handle,m_nAxisList[0],50); //设置加减速 S 曲线时间
        // }
 

      }
     
      static zmotion& getinstance()
      {
          static zmotion zm;
          return zm;

      }
      ~zmotion(){
       // int rtn = ZAux_Close( handle); //关闭连接,释放句柄
      //if(CheckError(rtn,"ZAux_Close")) return;
     // handle = NULL;

      }

 

   };


   class zmotionmotor:Motor
   { 

     zmotion& zm=zmotion::getinstance();
     public:
     int motor;
     zmotionmotor(int motor_id):motor(motor_id)
     {    
       
     }
        int Set2DGalvanometerPositon(float x,float y) override
         {
             std::cout<<"设置激光器位置"<<std::endl;
            //  float m_PosList[2]={x,y};
            //  int ret = ZAux_Direct_MoveAbs(zm.handle,2,zm.m_nAxisList,m_PosList); //运动到振镜原点
            //  return ret;
            return 1;
         }

         double actualPos() override
         {  
              //  float* p;
              //  int ret= ZAux_Direct_GetDpos(zm.handle,motor,p);
              //  return double(*p);
               std::cout<<"获取激光器位置"<<std::endl;
               return 1;
         }

   ~zmotionmotor()
   {
     
   }

   };

  class ZmotionTransceive:Transceive
  {
    private:
    public:
        auto init()->int override
        {
                 
           // urgazebo.init();
            return 1;
        }
        auto send(void)->void override
        {
        }
        auto  receive()->void override
        {
          
        }
  };
}
#endif
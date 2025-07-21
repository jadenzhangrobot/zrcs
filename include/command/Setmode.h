/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-29 13:54:20
 * @LastEditTime: 2023-04-20 15:48:24
 * @Description: 设置电机模式的指令
 * 
 */
#ifndef MODE_H_
#define MODE_H_
#include "system/basenodeInterface.h"
#include "system/centre.h"
#include <iostream>
class Setmode:public zrcsSystem::Basenode
{
   private:
      int motor_num;
      int motor_id;
      int motor_mode;
   public:
        Setmode()
        {
             port_input.add<int>("mode", 'o', "motor mode", false, 8, cmdline::range(0, 100));                    
        }
        void config() override
        {   
                   
            if (!cmdParam.empty()) 
            {
                  std::string str=cmdParam.front();
                  port_input.parse_check(str);
                  cmdParam.pop();
            } 
             motor_mode=port_input.get<int>("mode"); 
             // 添加空指针检查
             if(control)
             {
                 motor_num=control->axiss.size();
             }
             else
             {
                 motor_num=0;
                 std::cout << "Warning: control is null, setting motor_num to 0" << std::endl;
             }
        }
        void init() override
       {                
            node_status=RUNNING;
       }
        void  excuteRt(void) override
        {                     
                // 添加安全检查
                if(control && !control->axiss.empty())
                {
                    for(int i=0;i<motor_num;i++)
                    {
                        if(i >= 0 && i < static_cast<int>(control->axiss.size()) && control->axiss[i] != nullptr)
                        {
                            control->axiss[i]->setModeOfOperation(motor_mode);
                        }
                        else
                        {
                            std::cout << "Error: Invalid axis index " << i << " or null pointer" << std::endl;
                            node_status=FAILED;
                            return;
                        }
                    }
                }
                else
                {
                    std::cout << "Error: control is null or axiss is empty" << std::endl;
                    node_status=FAILED;
                    return;
                }
                node_status=SUCCESS;
        }  
         void exit()override
         {
             //rt_printf("设置模式成功\n");
             node_status=EXIT;
         }                
            
    
};
REGISTERCMD(Setmode);
#endif
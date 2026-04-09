
#pragma once

#include "system/base/BaseNodeInterface.h"
#include "system/centre.h"
#include <cstdint>
#include <iostream>
class Io:ZrcsSystem::Basenode
{
   private:
      uint16_t value;
   public:
        Io(const std::string& node_name="Io")
        {
         

    
        }
         void init() override
       {   
           port_input.add<uint16_t>("value", 'm', "motornumber", false, 0, cmdline::range(000, 65535));
        
          if (!CmdParam->empty()) 
          {
              std::string str=CmdParam->front();
              port_input.parse_check(str);
              CmdParam->pop();
          }   
                    
              value=port_input.get<uint16_t>("value");
              node_status=RUNNING;                 
          }


        void  excute_rt(void) override
        {            
                    Control->Ios[0]->write( value);
                     node_status=SUCCESS;
                                              
        }
      void exit(void) override
      {
           std::cout<<"IO写执行成功"<<std::endl;
      }



};
 REGISTER(Io);
 

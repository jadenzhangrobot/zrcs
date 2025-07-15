#ifndef REGISTER_H_
#define REGISTER_H_
#include "common/Shared memory/sharedData.h"
#include "common/Shared memory/registerInfo.h"
#include <stdexcept> 
namespace zrcsSystem
{

                    
      inline  void setOutputIo(SharedBlock* shared_block,RegisterType address,bool value)
        {
            if (address<OUTPUTIOSIZE&&address>=0) 
            {
                shared_block->registers.outputIo[address].store(value);
            }
            else 
            {
               throw std::runtime_error("address out of range");
            }
           
        }
      inline  bool getOutputIo(SharedBlock* shared_block,RegisterType address)noexcept(false)
        { 
            if (address<OUTPUTIOSIZE&&address>=0) 
            {
               return shared_block->registers.outputIo[address].load();
            }
            else 
            {
               throw std::runtime_error("address out of range");
            }
        }

      inline  void setInputIo(SharedBlock* shared_block,RegisterType address,bool value)
        {
            if (address<INPUTIOSIZE&&address>=0) 
            {
                shared_block->registers.inputIo[address].store(value);
            }
            else 
            {
               throw std::runtime_error("address out of range");
            }
        }
     inline  bool getInputIo(SharedBlock* shared_block,RegisterType address)noexcept(false)
        { 
            if (address<INPUTIOSIZE&&address>=0) 
            {
               return shared_block->registers.inputIo[address].load();
            }
            else 
            {
               throw std::runtime_error("address out of range");
            }
        }   
}
#endif
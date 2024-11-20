#ifndef NODECOMMUNICATION_H
#define NODECOMMUNICATION_H
#include <cstdint>
#include <memory_resource>
#include <unistd.h>
#include <vector>
  #define OUTPUT 1
  #define INPUT -1
namespace zrcsSystem {
  
    template<class T>
    class NodeCommunicaion
    {   private:
         std::pmr::monotonic_buffer_resource* resource;
         std::pmr::vector<T>* vec;
         public:
         NodeCommunicaion(std::string ,int direction, int size)
         {
                       resource = new std::pmr::monotonic_buffer_resource(size);
                       vec = new std::pmr::vector<T>(resource);
         }
          int size()
             {
                return vec->size();
             }
          int read(T& value)
          {  
              T result;
              if (!vec->empty()) 
              {
                 result=vec->front();
                 value=result;
                 vec->erase(vec->begin());
              }
              result -1;
          }
          void wirte(T value)
          {
                 vec->push_back(value);
          }

         ~NodeCommunicaion()
         {    
            delete vec; 
            delete resource;              
         }
         

    };
}
#endif
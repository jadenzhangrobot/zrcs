#ifndef ETHERCATSLAVEPDO_H
#define ETHERCATSLAVEPDO_H
#include "tinyxml2.h"
class EthercatSlves
{
     tinyxml2::XMLDocument doc;
     public:

     int init (void)
     {
        if (doc.LoadFile("example.xml") == tinyxml2::XML_SUCCESS) {

         
       } else {
    
         return -1;
       }
       return 1;
     }
};

#endif
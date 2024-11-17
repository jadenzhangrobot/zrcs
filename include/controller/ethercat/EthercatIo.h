#ifndef ETHERCATIO_H
#define ETHERCATIO_H
#include "controller/ControllerInterface.h"
#include "EthercatMaster.h"
#include <cstdint>
#include <ecrt.h>
#define SET_BIT(num, bitPos, value) \
    do { \
        if (value) { \
            num |= (1 << bitPos); \
        } else { \
            num &= ~(1 << bitPos); \
        } \
    } while(0)
#define GET_BIT(num, pos) ((num >> pos) & 1)
namespace HWAL{
    class EthercatIo:Io
    {
         
         private:
         EthercatMaster* ethercatMaster;
         int SlaveId;
        public:
         int offerset1;
         int offerset2;
         EthercatIo(int id,EthercatMaster* ethercatMaster_):SlaveId(id),ethercatMaster( ethercatMaster_)
         {
            //  offerset1= findNumberByKey(em.OutputPdoInfoAndOffset,std::to_string(1)+std::to_string(0x7000)+std::to_string(0x1));

            //  offerset1= findNumberByKey(em.OutputPdoInfoAndOffset,std::to_string(1)+std::to_string(0x7000)+std::to_string(0x2));
		 
        };
        //  int findNumberOutputKey(const std::string& key)
		// 	{
		// 		auto it = ethercatMaster->OutputPdoInfoAndOffset[motor_id].find(key);
		// 		if (it !=  ethercatMaster->OutputPdoInfoAndOffset[motor_id].end()) {
		// 			return it->second;
		// 		} else {
		// 			throw std::runtime_error("Failed to find "+std::to_string(motor_id)+key);
		// 		}
        //     }
		// 	 int findNumberInputKey(const std::string& key)
		// 	{
		// 		auto it = ethercatMaster->InputPdoInfoAndOffset[motor_id].find(key);
		// 		if (it != ethercatMaster->InputPdoInfoAndOffset[motor_id].end()) {
		// 			return it->second;
		// 		} else {
		// 			throw std::runtime_error("Failed to find "+std::to_string(motor_id)+key);
		// 		}
        //     }
          int Write(std::string reg, int type, int bitPos,bool value)override
          {
                auto it = ethercatMaster->OutputPdoInfoAndOffset[SlaveId].find(reg);
				if (it !=  ethercatMaster->OutputPdoInfoAndOffset[SlaveId].end()) 
                {				
                    switch (type)
                    {
                        case 8:
                            {
                                uint16_t result16= EC_READ_U16(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[SlaveId][it->second]);
                                SET_BIT(result16,bitPos,value);
                                EC_WRITE_U16(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[SlaveId][it->second], result16 ); 
                            }
                        break;
                        case 16:
                            {
                            uint8_t result8= EC_READ_U8(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[SlaveId][it->second]);
                            SET_BIT(result8,bitPos,value);
                            EC_WRITE_U8(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[SlaveId][ it->second],result8);
                            }
                        break;
                        case 32:
                            {
                            uint32_t result32= EC_READ_U32(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[SlaveId][it->second]);
                            SET_BIT(result32,bitPos,value);
                            EC_WRITE_U32(ethercatMaster->DomainWrite+ethercatMaster->OutputOffset[SlaveId][ it->second],result32);
                            }
                        break;
                        default:
                            return -1;
                        break;               
                 }
                }else {
                   return -1;
                }
               return 0;
          }
          int Read(std::string reg, int type, int bitPos) override
          {

                auto it = ethercatMaster->InputPdoInfoAndOffset[SlaveId].find(reg);
				if (it !=  ethercatMaster->InputPdoInfoAndOffset[SlaveId].end()) 
                {	
                    switch (type)
                    {
                        case 8:
                            {
                                uint8_t result8= EC_READ_U8(ethercatMaster->DomainRead+ethercatMaster->InputOffset[SlaveId][it->second]);
                                return  GET_BIT(result8,bitPos);
                            }
                        break;
                        case 16:
                            {
                                uint16_t result16= EC_READ_U16(ethercatMaster->DomainRead+ethercatMaster->InputOffset[SlaveId][it->second]);
                                return GET_BIT(result16,bitPos);
                            }
                        break;
                        case 32:
                            {
                               uint32_t result32= EC_READ_U32(ethercatMaster->DomainRead+ethercatMaster->InputOffset[SlaveId][it->second]);
                               return GET_BIT(result32,bitPos);
                            }
                        break;
                        default:
                            return -1;
                        break;          
                    }
                }else {
                  return -1;
                }
            return 0;
          }
            
        
         ~EthercatIo(){} ;
    };

}
#endif
#pragma once

#include "controller/Io.h"
#include "EthercatMaster.h"
#include <ecrt.h>
#include <cstdint>
#include <type_traits>
#define SET_BIT(num, bitPos, value) \
    do { \
        if (value) { \
            num |= (1 << bitPos); \
        } else { \
            num &= ~(1 << bitPos); \
        } \
    } while(0)
#define GET_BIT(num, pos) ((num >> pos) & 1)

namespace ZrcsHardware {
    class EthercatIo: public Io
    {         
         private: 
         EthercatMaster* ethercatMaster;
       
         int SlaveId;
        public:
         EthercatIo(int id,EthercatMaster* ethercatMaster_):SlaveId(id),ethercatMaster( ethercatMaster_)
         {
            
         };
      
        void ioWrite8(int index, int bitPos, bool value) override
        {
            uint8_t current = EC_READ_U8(ethercatMaster->DomainWrite + ethercatMaster->OutputOffset[SlaveId][index]);
            SET_BIT(current, bitPos, value);
            EC_WRITE_U8(ethercatMaster->DomainWrite + ethercatMaster->OutputOffset[SlaveId][index], current);
        }

        void ioWrite16(int index, int bitPos, bool value) override
        {
            uint16_t current = EC_READ_U16(ethercatMaster->DomainWrite + ethercatMaster->OutputOffset[SlaveId][index]);
            SET_BIT(current, bitPos, value);
            EC_WRITE_U16(ethercatMaster->DomainWrite + ethercatMaster->OutputOffset[SlaveId][index], current);
        }

        void ioWrite32(int index, int bitPos, bool value) override
        {
            uint32_t current = EC_READ_U32(ethercatMaster->DomainWrite + ethercatMaster->OutputOffset[SlaveId][index]);
            SET_BIT(current, bitPos, value);
            EC_WRITE_U32(ethercatMaster->DomainWrite + ethercatMaster->OutputOffset[SlaveId][index], current);
        }

       
        bool ioRead8(int index, int bitPos) override
        {
            uint8_t result = EC_READ_U8(ethercatMaster->DomainRead + ethercatMaster->InputOffset[SlaveId][index]);
            return GET_BIT(result, bitPos);
        }

        bool ioRead16(int index, int bitPos) override
        {
            uint16_t result = EC_READ_U16(ethercatMaster->DomainRead + ethercatMaster->InputOffset[SlaveId][index]);
            return GET_BIT(result, bitPos);
        }

        bool ioRead32(int index, int bitPos) override
        {
            uint32_t result = EC_READ_U32(ethercatMaster->DomainRead + ethercatMaster->InputOffset[SlaveId][index]);
            return GET_BIT(result, bitPos);
        }

        void aoWriteValue(int index, double value) override
        {
            auto raw = static_cast<int16_t>(value);
            EC_WRITE_S16(ethercatMaster->DomainWrite + ethercatMaster->OutputOffset[SlaveId][index], raw);
        }

        double aoReadValue(int index) override
        {
            auto raw = EC_READ_S16(ethercatMaster->DomainRead + ethercatMaster->InputOffset[SlaveId][index]);
            return static_cast<double>(raw);
        }

         ~EthercatIo(){} ;
    };

}

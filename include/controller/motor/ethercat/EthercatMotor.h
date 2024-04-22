
#ifndef ETHERCATMOTOR_H
#define ETHERCATMOTOR_H
#include "controller/controller_interface.h"
#include "EthercatMaster.h"
#include <cstdint>
#include <cstdio>
#include <ecrt.h>
namespace controller {
class EthercatMotor:Motor
{
      EthercatMaster em=EthercatMaster::getInstance();
     public:
     int motor_id;
        EthercatMotor(int id):motor_id(id)
        {

        }

		 int mode(std::uint8_t md) override
          {

               EC_WRITE_S8(em.domain1_pd + em.offset.operation_mode[motor_id], 0x8);			 
               return 1;
          }
        int setTargetPos (double position) override
        {
             int32_t position_=(int32_t)(1000*position);
			// int32_t position_=(int32_t)(1048576*position);
             EC_WRITE_S32(em.domain1_pd + em.offset.target_position[motor_id],position_);
             return 1;  
        }
        double actualPos(void) override
        {
              std::int32_t pos= EC_READ_S32(em.domain1_pd + em.offset.current_position[motor_id]);
			 // double pos_=(double)pos/1048576;
			 double pos_=(double)pos/1000;
			  return pos_;
        }
        double actualVel(void) override
        {
           return 0;
        }
        int  setTargetToq(double toq) override
        {  
			return 0;
        }
        double actualToq(void) override
        {
              return 0;
        }
       
           std::uint16_t controlWord() override
         {
                
               return EC_READ_U16(em.domain1_pd + em.offset.ctrl_word[motor_id]);
         }

         void setControlWord(std::uint16_t control_word) override
         {
                EC_WRITE_U16(em.domain1_pd + em.offset.ctrl_word[motor_id], control_word ); 

         }

          std::uint16_t statusWord() override
         {
              
              return EC_READ_U16(em.domain1_pd + em.offset.status_word[motor_id]);

         }

          int  disable() override
          {
             // if (imp_->slave_->isVirtual()) imp_->status_word_ = 0x40;

		// control word
		// 0x06    0b xxxx xxxx 0xxx 0110    A: transition 2,6,8         Shutdown
		// 0x07    0b xxxx xxxx 0xxx 0111    B: transition 3             Switch ON
		// 0x0F    0b xxxx xxxx 0xxx 1111    C: transition 3             Switch ON
		// 0x00    0b xxxx xxxx 0xxx 0000    D: transition 7,9,10,12     Disable Voltage
		// 0x02    0b xxxx xxxx 0xxx 0000    E: transition 7,10,11       Quick Stop
		// 0x07    0b xxxx xxxx 0xxx 0111    F: transition 5             Disable Operation
		// 0x0F    0b xxxx xxxx 0xxx 1111    G: transition 4,16          Enable Operation
		// 0x80    0b xxxx xxxx 1xxx xxxx    H: transition 15            Fault Reset
		//
		// status word
		// 0x00    0b xxxx xxxx x0xx 0000    A: not ready to switch on     
		// 0x40    0b xxxx xxxx x1xx 0000    B: switch on disabled         
		// 0x21    0b xxxx xxxx x01x 0001    C: ready to switch on         
		// 0x23    0b xxxx xxxx x01x 0011    D: switch on                  
		// 0x27    0b xxxx xxxx x01x 0111    E: operation enabled          
		// 0x07    0b xxxx xxxx x00x 0111    F: quick stop active
		// 0x0F    0b xxxx xxxx x0xx 1111    G: fault reaction active
		// 0x08    0b xxxx xxxx x0xx 1000    H: fault
		//
		// 0x6F    0b 0000 0000 0110 1111
		// 0x4F    0b 0000 0000 0100 1111
		// disable change state to A/B/C/E to D

		auto status_word = statusWord();
         
		// check status A, now transition 1 automatically
		if ((status_word & 0x4F) == 0x00) {
			// this just set the initial control word...
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x00));
			setControlWord(std::uint16_t(0x00));
			return 1;
		}
		// check status B, now keep and return
		else if ((status_word & 0x4F) == 0x40) {
			// transition 2 //
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x00));
			setControlWord(std::uint16_t(0x00));
			return 0;
		}
		// check status C, now transition 7
		else if ((status_word & 0x6F) == 0x21) {
			// transition 3 //
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x00));
			setControlWord(std::uint16_t(0x00));
			return 0;
		}
		// check status D, now transition 10
		else if ((status_word & 0x6F) == 0x23) {
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x06));//change to 0x06 for cooldrive
			setControlWord(std::uint16_t(0x06));//change to 0x06 for cooldrive
			return 3;
		}
		// check status E, now transition 9
		else if ((status_word & 0x6F) == 0x27) {
			// transition 5 //
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x07));//change to 0x07 for cooldrive
			setControlWord(std::uint16_t(0x07));//change to 0x07 for cooldrive
			return 4;
		}
		// check status F, now transition 12
		else if ((status_word & 0x6F) == 0x07) {
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x00));
			setControlWord(std::uint16_t(0x00));
			return 5;
		}
		// check status G, now transition 14
		else if ((status_word & 0x4F) == 0x0F) {
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x00));
			setControlWord(std::uint16_t(0x00));
			return 6;
		}
		// check status H, now transition 15
		else if ((status_word & 0x4F) == 0x08) {
			// transition 4 //
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x80));
			setControlWord(std::uint16_t(0x80));
			return 7;
		}
		// unknown status
		else {
			return -1;
		}
          }
           int enable() override
          {
              // if (imp_->slave_->isVirtual()) imp_->status_word_ = 0x27;

		// control word
		// 0x06    0b xxxx xxxx 0xxx 0110    A: transition 2,6,8       Shutdown
		// 0x07    0b xxxx xxxx 0xxx 0111    B: transition 3           Switch ON
		// 0x0F    0b xxxx xxxx 0xxx 1111    C: transition 3           Switch ON
		// 0x00    0b xxxx xxxx 0xxx 0000    D: transition 7,9,10,12   Disable Voltage
		// 0x02    0b xxxx xxxx 0xxx 0000    E: transition 7,10,11     Quick Stop
		// 0x07    0b xxxx xxxx 0xxx 0111    F: transition 5           Disable Operation
		// 0x0F    0b xxxx xxxx 0xxx 1111    G: transition 4,16        Enable Operation
		// 0x80    0b xxxx xxxx 1xxx xxxx    H: transition 15          Fault Reset
		// 
		// status word
		// 0x00    0b xxxx xxxx x0xx 0000    A: not ready to switch on     
		// 0x40    0b xxxx xxxx x1xx 0000    B: switch on disabled         
		// 0x21    0b xxxx xxxx x01x 0001    C: ready to switch on         
		// 0x23    0b xxxx xxxx x01x 0011    D: switch on                  
		// 0x27    0b xxxx xxxx x01x 0111    E: operation enabled          
		// 0x07    0b xxxx xxxx x00x 0111    F: quick stop active
		// 0x0F    0b xxxx xxxx x0xx 1111    G: fault reaction active
		// 0x08    0b xxxx xxxx x0xx 1000    H: fault
		// 
		// 0x6F    0b 0000 0000 0110 1111
		// 0x4F    0b 0000 0000 0100 1111
		// enable change state to A/B/C/D/F/G/H to E

		auto status_word = statusWord();
		// check status A
		if ((status_word & 0x4F) == 0x00) {
			return 1;
		}
		// check status B, now transition 2
		else if ((status_word & 0x4F) == 0x40) {
			// transition 2 //
			
			setControlWord(std::uint16_t(0x06));
			return 2;
		}
		// check status C, now transition 3
		else if ((status_word & 0x6F) == 0x21) {
			// transition 3 //
			
			setControlWord(std::uint16_t(0x07));
			return 3;
		}
		// check status D, now transition 4
		else if ((status_word & 0x6F) == 0x23) {
			// transition 4 //
		   
					setControlWord(std::uint16_t(0x0F));
					// check mode to set correct pos, vel or cur //
					switch (0x08) 
					{
					case 0x08: setTargetPos(actualPos()); break;
				
					default: setTargetPos(actualPos());
					}
			return 4;
		}
		// check status E, now keep status
		else if ((status_word & 0x6F) == 0x27) {
			// check if need wait //
			// if (--imp_->waiting_count_left > 0) return 5;
			// // now return normal
			// else return 5;
            return 5;
		}
		// check status F, now transition 12
		else if ((status_word & 0x6F) == 0x07) {
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x00));
			setControlWord(std::uint16_t(0x00));
			return 6;
		}
		// check status G, now transition 14
		else if ((status_word & 0x4F) == 0x0F) {
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x00));
			setControlWord(std::uint16_t(0x00));
			return 7;
		}
		// check status H, now transition 15
		else if ((status_word & 0x4F) == 0x08) {
			// transition 4 //
			//imp_->slave_->writePdo(0x6040, 0x00, std::uint16_t(0x80));
			setControlWord(std::uint16_t(0x80));
			return 8;
		}
		// unknown status
		else
		{
			return -1;
		}
          }


    };
 class EthercatTransceive:Transceive
  {
    private:
      EthercatMaster em=EthercatMaster::getInstance();
    public:
         auto init()->int override
        {
                 
            em.EthercatInit();
            return 1;
        }
        auto send(void)->void override
        {
           em.SendData();
        }
        auto  receive()->void override
        {
            em.ReceiveData();
        }
};

}


#endif

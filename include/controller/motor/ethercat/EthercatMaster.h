#ifndef ETHERCATMASTER
#define ETHERCATMASTER
#include "ecrt.h"
#include <iostream>
#include <alchemy/task.h> 
#include <alchemy/timer.h> 
#include <rtdm/rtdm.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>

namespace controller {
#define slaves 1 //slave number

#define DM3E         0                     /*EtherCAT address on the bus*/
#define VID  0x00000083
#define PID  0x00000005  /*Vendor ID, product code*/


#define DC_FILTER_CNT          1024
#define sign(val) \
    ({ typeof (val) _val = (val); \
    ((_val > 0) - (_val < 0)); })

class EthercatMaster {
  private:
 static inline uint cycle_ns=1000000;

  static inline ec_master_t *master = NULL;

   

 //static inline ec_pdo_entry_reg_t domain1_regs[slaves*7];
static inline  ec_master_state_t master_state = {};

static inline  ec_domain_t *domain1 = NULL;
static inline  ec_domain_state_t domain1_state = {};

static inline  ec_slave_config_t *sc;
static inline  ec_slave_config_state_t sc_state = {};

 
 
 static inline uint64_t dc_start_time_ns = 0LL;
 static inline uint64_t dc_time_ns = 0;

 static inline uint8_t  dc_started = 0;
 static inline int32_t  dc_diff_ns = 0;
 static inline int32_t  prev_dc_diff_ns = 0;
 static inline int64_t  dc_diff_total_ns = 0LL;
 static inline  int64_t  dc_delta_total_ns = 0LL;
 static inline int      dc_filter_idx = 0;
 static inline  int64_t  dc_adjust_ns;

static inline  int64_t  system_time_base = 0LL;
static inline  uint64_t wakeup_time = 0LL;
static inline  uint64_t overruns = 0LL;
static inline ec_pdo_entry_info_t device_pdo_entries[7] = {
    /*RxPdo 0x1600*/
    {0x6040, 0x00, 16},
    {0x6060, 0x00, 8 }, 
    {0x60FF, 0x00, 32},
    {0x607A, 0x00, 32},
    /*TxPdo 0x1A00*/
    {0x6041, 0x00, 16},
    {0x606C, 0x00, 32},
    {0x6064, 0x00, 32}
};

 static inline ec_pdo_info_t device_pdos[2] = {
    //RxPdo
    {0x1600, 4, device_pdo_entries + 0 },
    //TxPdo
    {0x1A00, 3, device_pdo_entries + 4}
};

static inline ec_sync_info_t device_syncs[5] = {
    { 0, EC_DIR_OUTPUT, 0, NULL, EC_WD_ENABLE },
    { 1, EC_DIR_INPUT, 0, NULL, EC_WD_ENABLE },
    { 2, EC_DIR_OUTPUT, 1, device_pdos + 0, EC_WD_ENABLE },
    { 3, EC_DIR_INPUT, 1, device_pdos + 1, EC_WD_ENABLE},
    { 0xFF}
};
uint64_t system_time_ns(void)
{
   RTIME time = rt_timer_read();

      
    if ((system_time_base>0)&&((uint64_t)system_time_base>time)) 
    {
        
        rt_printf("%s() error: system_time_base greater than"
                " system time (system_time_base: %lld, time: %llu\n",
                __func__, system_time_base, time);
        return time;
    }
    else 
    {
        return time - system_time_base;
    }
}
SRTIME system2count(uint64_t time)
{   
    SRTIME ret;

      if ((system_time_base < 0) &&
           ((uint64_t) (-system_time_base) > time)) {
           rt_printf("%s() error: system_time_base less than"
         " system time (system_time_base: %lld, time: %llu\n",
                __func__, system_time_base, time);
          ret = time;
         }  
      else
         {
          ret = time+system_time_base;
         }
    return rt_timer_ns2ticks(ret);
    
     
}
  void wait_period(void)
{
    while (1)
    {
        SRTIME wakeup_count = system2count(wakeup_time);
        RTIME current_count = rt_timer_read();

        if ((wakeup_count < current_count)|| (wakeup_count > current_count + (50 * cycle_ns))) 
        {
            rt_printf("%s(): unexpected wake time!\n", __func__);
           // exit(1);
        }

        switch (rt_task_sleep_until(wakeup_count)) 
        {
            case EPERM:
                rt_printf("rt_sleep_until(): RTE_UNBLKD\n");
                continue;

            case ETIMEDOUT:
                rt_printf("rt_sleep_until(): RTE_TMROVRN\n");
                overruns++;

                if (overruns % 100 == 0)
              {
                    // in case wake time is broken ensure other processes get
                    // some time slice (and error messages can get displayed)
                    rt_task_sleep(cycle_ns / 100);
               }
                break;

            default:
                break;
        }

        // done if we got to here
        break;
    }

    ecrt_master_application_time(master, wakeup_time);


    // calc next wake time (in sys time)
    wakeup_time += cycle_ns;
    //rt_printf("wakeup_time=%ld\n",wakeup_time);

}
void update_master_clock(void)
{

    // calc drift (via un-normalised time diff)
    int32_t delta = dc_diff_ns - prev_dc_diff_ns;
    prev_dc_diff_ns = dc_diff_ns;
     
    dc_diff_ns =((dc_diff_ns + (cycle_ns / 2)) % cycle_ns) - (cycle_ns / 2);
    dc_diff_total_ns += dc_diff_ns;
    dc_delta_total_ns += delta;
   
   
  
        // add to totals
        dc_diff_total_ns += dc_diff_ns;
        dc_delta_total_ns += delta;
        dc_filter_idx++;

        if (dc_filter_idx >= DC_FILTER_CNT) 
      {
            // add rounded delta average
            dc_adjust_ns +=((dc_delta_total_ns + (DC_FILTER_CNT / 2)) / DC_FILTER_CNT);

            // and add adjustment for general diff (to pull in drift)
            dc_adjust_ns += sign(dc_diff_total_ns / DC_FILTER_CNT);
            
            // limit crazy numbers (0.1% of std cycle time)
            if (dc_adjust_ns < -1000) 
            {
               dc_adjust_ns = -1000;
            }
            if (dc_adjust_ns > 1000) 
            {
               dc_adjust_ns =  1000;
            }
            // reset
            dc_diff_total_ns = 0LL;
            dc_delta_total_ns = 0LL;
            dc_filter_idx = 0;
      }
   
        // add cycles adjustment to time base (including a spot adjustment)
        system_time_base += dc_adjust_ns + sign(dc_diff_ns);
        
        //rt_printf("system_time_base%lld\n",system_time_base);
  
}


//获取不同周期下apptime的和参考时间ref_time的误差
void sync_distributed_clocks(void)
{

    uint32_t ref_time=0;
    uint64_t prev_app_time =dc_time_ns;


    static uint32_t last_ref_time=0;
    static uint64_t u64_reftime=0;

    static uint64_t i__=0;
    static uint64_t last_diff=0;
    static int32_t period_max_time=0;
    static  int32_t  period_min_time=0;
       dc_time_ns = system_time_ns();
      // set master time in nano-seconds
     // get reference clock time to synchronize master cycle
     ecrt_master_reference_clock_time(master, &ref_time);
     
    
     //rt_printf("prev_app_time %u\n",(uint32_t)prev_app_time);
     dc_diff_ns =(uint32_t)prev_app_time-ref_time;
     if(ref_time<last_ref_time)
     {
      u64_reftime=u64_reftime+(ref_time+4294967296-last_ref_time);

     }
     else
      {
        u64_reftime=u64_reftime+(ref_time-last_ref_time);
         
      }
     uint32_t diff=u64_reftime%1000000;
     last_ref_time=ref_time; 
     
                   if(i__<900)
                      {
                       period_min_time=diff;
                      } 

                    
                    if(i__>10000)
		      
		      {
			     if(diff>period_max_time)
			     {
				 period_max_time=diff;
		
			     }
			    
			     if((diff<period_min_time)&&( period_min_time>0))
			     {
				 period_min_time=diff;
			    	 				
			     }
                      
                      }
        //    rt_printf("period_max_time %lld\n",period_max_time);
		//        rt_printf("period_min_time %lld\n",period_min_time);
        //    rt_printf("\n");
        //    rt_printf("diff   %u\n",diff);
        //    rt_printf("\n");
        //    rt_printf("u64_reftime %llu\n",u64_reftime);
        //    rt_printf("\n");       
                	
			last_diff=diff;      
     i__++;
     // call to sync slaves to ref slave
     ecrt_master_sync_slave_clocks(master);         
}


  public:

 static inline   uint8_t *domain1_pd = NULL;


 
  typedef struct{
    unsigned int operation_mode[slaves];
    unsigned int ctrl_word[slaves];
    unsigned int target_velocity[slaves];
    unsigned int target_position[slaves];
    unsigned int status_word[slaves];
    unsigned int current_velocity[slaves];
    unsigned int current_position[slaves];
}offset1;
  static inline offset1 offset;
     ec_pdo_entry_reg_t domain1_regs[8] = {
    {DM3E,0, VID,PID, 0x6040, 0,&offset.ctrl_word[0]},
    {DM3E,0, VID,PID, 0x6060, 0, &offset.operation_mode[0]},
    {DM3E,0, VID,PID, 0x60FF, 0, &offset.target_velocity[0]},
    {DM3E,0, VID,PID, 0x607A, 0, &offset.target_position[0]},
    {DM3E,0, VID,PID, 0x6041, 0, &offset.status_word[0]},
    {DM3E,0, VID,PID, 0x606C, 0, &offset.current_velocity[0]},
    {DM3E,0, VID,PID, 0x6064, 0, &offset.current_position[0]},
    {}
};
  EthercatMaster() {

    
  }
  ~EthercatMaster()
  {
    ecrt_release_master(master);
  }
 static EthercatMaster& getInstance(void)
  {

          static EthercatMaster em;
          return em;
 
  }
  int EthercatInit()
  {
  

    master = ecrt_request_master(0);
    if (master == nullptr) {
      std::cout << "获取主站失败" << std::endl;
      return -1;
    }

    domain1 = ecrt_master_create_domain(master);
    if (domain1 == nullptr) {
      std::cout << "创建domain失败" << std::endl;
      return -1;
    }
    for(int i=0;i<slaves;i++)
    {
	   if (!(sc = ecrt_master_slave_config(master,DM3E,i, VID,PID)))
	    {
		    fprintf(stderr, "Failed to get slave configuration for slave!\n");
		    exit(EXIT_FAILURE);
	    }
	    printf("Configuring PDOs...\n");
    
	    if (ecrt_slave_config_pdos(sc, EC_END, device_syncs)!=0)
	    {
	       fprintf(stderr, "Failed to configure slave PDOs!\n");
	       exit(EXIT_FAILURE);
	    }
	    else
	    {
		    printf("*Success to configuring slave PDOs*\n");
	    }
	    if(i==0)
	    {
	        ecrt_master_select_reference_clock(master,sc);
	    } 
	     ecrt_slave_config_dc(sc,0x0300,cycle_ns,300000,0,0);
    }
   
         
    if (ecrt_domain_reg_pdo_entry_list(domain1, domain1_regs)) 
    {
        fprintf(stderr, "PDO entry registration failed!\n");
        exit(EXIT_FAILURE);
    }
    
    dc_start_time_ns = system_time_ns();
    dc_time_ns = dc_start_time_ns;

  
    if (ecrt_master_activate(master)<0)
    {
        return -1;
    }
   
    if ((domain1_pd = ecrt_domain_data(domain1))==nullptr)
    {
         printf("ecrt_domain_data*\n");
       return -1;
    }
    return 1;
  }



    void SendData()
      {  // int err = rt_task_set_periodic(NULL,TM_NOW, 1000000); 
        
           ecrt_domain_queue(domain1);   
         
           sync_distributed_clocks();
           ecrt_master_send(master); 
           update_master_clock();  

          // ecrt_master_application_time(master, rt_timer_read());
			    // ecrt_master_sync_reference_clock(master);
			    // ecrt_master_sync_slave_clocks(master);   
      }

      void ReceiveData()
      {
        static int flag=0;
        if (flag==0) {
             wakeup_time = system_time_ns() + 10 * cycle_ns;
             flag=1;
        }
              
           wait_period();
          ecrt_master_receive(master);
          ecrt_domain_process(domain1); 
         

      }
};
}
#endif

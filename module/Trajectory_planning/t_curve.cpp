#include "t_curve.h"

 
  void t_curve_init(struct t_curve_data* data, double c_p, double t_p, double vm, double am) 
   {
        data->t0=0;
        data->current_p=c_p;
        data->target_p=t_p;
        data->v_max=vm;
        data->a_max=am;
        data->ta=vm/am;//加减速需要的时间
        data->pa=0.5*data->a_max*(data->ta*data->ta);//加速或者减速产生的位置量
        data->tm=((data->target_p-data->current_p)-2*data->pa)/data->v_max;//最大速度需要的时间
        data->t_all=2*data->ta+data->tm;//总共需要的时间
   }
    double t_curve(struct t_curve_data* data, double t)
  {
        //达到最大速度，梯形
        if (data->t_all-2*data->ta>0)
        {
                if (t<=data->ta) {
                  return  data->current_p+0.5*data->a_max*(t*t);
                }
                else if (data->ta<t&&t<=data->t_all-data->ta) {
                  return data->current_p+0.5*data->a_max*data->ta*data->ta+data->a_max*data->ta*(t-data->ta);
                }
                else {
                 return data->target_p-0.5*data->a_max*(data->t_all-t)*(data->t_all-t);
                }
          
        }
        //未达到最大速度速度曲线为三角型
        else {
           data->ta=sqrt( (data->target_p-data->current_p)/data->a_max);
           data->t_all=2*data->ta;
             if (t<=data->ta)
              {
                 return data->current_p+0.5*data->a_max*t*t;
              }
             else 
             {
                 return data->target_p-0.5*data->a_max*((data->t_all-t)*(data->t_all-t));
             }      
        }
}         

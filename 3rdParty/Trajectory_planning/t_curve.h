/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-09-7
 * @LastEditTime: 2023-09-07 
 * @Description: T型速度规划
 */
#ifndef TEST_H
#define TEST_H
#include<stdio.h>
#include "math.h"
struct t_curve_data{
    double t0;
    double current_p;//当前位置
    double target_p;//目标位置
    double v_max;//最大速度
    double a_max;//最大加速度
    double ta;//加减速需要的时间
    double pa;//加速或者减速产生的位置量
    double tm;//最大速度需要的时间
    double t_all;//总共需要的时间
};
   //初始化函数
   //t_curve_data 数据结构体
   // c_p 当前位置
   // t_p 目标位置
   // vm  最大速度
   // am  最大加速度
  void t_curve_init(struct t_curve_data* data, double c_p, double t_p, double vm, double am);
  //计算函数
  //t_curve_data 数据结构体
  // 运动时间
  double t_curve(struct t_curve_data* data,double t);       

#endif
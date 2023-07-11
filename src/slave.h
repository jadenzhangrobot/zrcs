/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-22 17:25:22
 * @LastEditTime: 2023-06-20 13:38:43
 * @Description: 
 * 
 * Copyright (c) 2023 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef SLAVE_H_
#define SLAVE_H_
#include <iostream>
#include <memory>
#include "../include/controller/controller_interface.h"
//宏定义整个系统的电机或者关节个数
#define JointNum 6
std::unique_ptr<controller::Controller> GazeboController(void);

std::unique_ptr<controller::Controller> InnfosController(void);

std::unique_ptr<controller::Controller> UrgazeboController(void);

std::unique_ptr<controller::Controller> GlrbusController(void);
#endif
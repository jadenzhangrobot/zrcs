/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: sensor hardware abstraction
 */
#pragma once

namespace ZrcsHardware {

/// 传感器抽象基类（预留扩展）。
class Sensor
{
public:
    virtual ~Sensor();
};

} // namespace ZrcsHardware

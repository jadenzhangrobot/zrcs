/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: io hardware abstraction
 */
#pragma once

namespace ZrcsHardware {

/// IO 抽象接口：数字量（bit 级读写）与模拟量（AO/AI）I/O 模块。
class Io
{
public:
    virtual ~Io();

    virtual bool isEmergencyStop();
    virtual void setEmergencyStop(bool value);

    virtual bool ioRead32(int index, int bitPos) = 0;
    virtual bool ioRead16(int index, int bitPos) = 0;
    virtual bool ioRead8(int index, int bitPos) = 0;
    virtual void ioWrite32(int index, int bitPos, bool value) = 0;
    virtual void ioWrite16(int index, int bitPos, bool value) = 0;
    virtual void ioWrite8(int index, int bitPos, bool value) = 0;

    virtual void aoWriteValue(int index, double value);
    virtual double aoReadValue(int index);
};

} // namespace ZrcsHardware

#pragma once

// 寄存器类型枚举，值对应 systemRegister::boolRegisters 数组的索引
enum class RegisterType : int
{
    STOP = 0,     // 停止寄存器
    PAUSE = 1,    // 暂停寄存器
    RESET = 2,    // 重置寄存器
    ENABLE = 3,   // 使能寄存器
    ALARM = 4,    // 报警寄存器

    // 寄存器总数（用于边界检查）
    REGISTER_COUNT = 32
};

#ifndef GLOBAL_H
#define GLOBAL_H
typedef enum 
{
    MC_ERRORCODE_GOOD                           = 0x0, //成功
    MC_ERRORCODE_QUEUEFULL                      = 0x1, //轴队列已满
    MC_ERRORCODE_AXISENCODEROVERFLOW            = 0x2, //轴编码器溢出
    MC_ERRORCODE_AXISPOWEROFF                   = 0x3, //轴未使能
    MC_ERRORCODE_AXISPOWERON                    = 0x4, //轴已功能
    MC_ERRORCODE_FREQUENCYILLEGAL               = 0x5, //频率不合法
    MC_ERRORCODE_AXISNOTEXIST                   = 0x8, //轴ID号不存在
    MC_ERRORCODE_AXISBUSY                       = 0xA, //轴正忙，有功能块正在控制轴运动
    MC_ERRORCODE_FAILEDTOBUFFER                 = 0xF, //不支持以buffer形式添加
    MC_ERRORCODE_BLENDINGMODEILLEGAL            = 0x10, //BufferMode值非法
    MC_ERRORCODE_PARAMETERNOTSUPPORT            = 0x14, //不支持该参数号
    MC_ERRORCODE_OVERRIDEILLEGAL                = 0x17, //OVERRIDE值非法
    MC_ERRORCODE_SHIFTINGMODEILLEGAL            = 0x19, //移动模式非法
    MC_ERRORCODE_SOURCEILLEGAL                  = 0x1A, //获取源非法
    MC_ERRORCODE_CONTROLMODEILLEGAL             = 0x23, //控制模式设置错误

    MC_ERRORCODE_POSILLEGAL                     = 0x100, //位置不合法
    MC_ERRORCODE_ACCILLEGAL                     = 0x101, //加/减速度不合法
    MC_ERRORCODE_VELILLEGAL                     = 0x102, //速度不合法
    MC_ERRORCODE_AXISHARDWARE                   = 0x103, //硬件错误
    MC_ERRORCODE_VELLIMITTOOLOW                 = 0x104, //由于配置文件限制，无法到达跟随的速度（电子齿轮，凸轮中）
    MC_ERRORCODE_ENDVELCANNOTREACH              = 0x105, //实际终速度过高无法到达预设速度

    MC_ERRORCODE_CMDPPOSOVERLIMIT               = 0x106, //指令位置超出配置文件正向限制
    MC_ERRORCODE_CMDNPOSOVERLIMIT               = 0x107, //指令位置超出配置文件负向限制
    MC_ERRORCODE_FORBIDDENPPOSMOVE              = 0x108, //禁止正向移动
    MC_ERRORCODE_FORBIDDENNPOSMOVE              = 0x109, //禁止负向移动

    MC_ERRORCODE_POSLAGOVERLIMIT                = 0x10A, //轴跟随误差超限
    MC_ERRORCODE_CMDVELOVERLIMIT                = 0x10B, //轴指令速度超出限制
    MC_ERRORCODE_CMDACCOVERLIMIT                = 0x10C, //轴指令加速度超出限制
    MC_ERRORCODE_POSINFINITY                    = 0x10E, //轴设定位置不合法
    
    MC_ERRORCODE_SOFTWAREEMGS                   = 0x1EE, //用户急停
    MC_ERRORCODE_SYSTEMEMGS                     = 0x1EF, //系统急停
    MC_ERRORCODE_COMMUNICATION                  = 0x1F0, //硬件通信异常

    /** 配置错误**/
    MC_ERRORCODE_CFGAXISIDILLEGAL               = 0x201,
    MC_ERRORCODE_CFGUNITRATIOOUTOFRANGE         = 0x202,
    MC_ERRORCODE_CFGCONTROLMODEILLEGAL          = 0x203,
    MC_ERRORCODE_CFGVELLIMITILLEGAL             = 0x204,
    MC_ERRORCODE_CFGACCLIMITILLEGAL             = 0x205,
    MC_ERRORCODE_CFGPOSLAGILLEGAL               = 0x206,
    MC_ERRORCODE_CFGPKPILLEGAL                  = 0x207,
    MC_ERRORCODE_CFGFEEDFORWORDILLEGAL          = 0x208,
    MC_ERRORCODE_CFGMODULOILLEGAL               = 0x209,
    
    MC_ERRORCODE_HOMINGVELILLEGAL               = 0x210,
    MC_ERRORCODE_HOMINGACCILLEGAL               = 0x211,
    MC_ERRORCODE_HOMINGSIGILLEGAL               = 0x212,
    MC_ERRORCODE_HOMINGMODEILLEGAL              = 0x214,
    MC_ERRORCODE_HOMEPOSITIONILLEGAL            = 0x215,

    MC_ERRORCODE_AXISDISABLED                   = 0x500,
    MC_ERRORCODE_AXISSTANDSTILL                 = 0x501,
    MC_ERRORCODE_AXISHOMING                     = 0x502,
    MC_ERRORCODE_AXISDISCRETEMOTION             = 0x503,
    MC_ERRORCODE_AXISCONTINUOUSMOTION           = 0x504,
    MC_ERRORCODE_AXISSYNCHRONIZEDMOTION         = 0x505,
    MC_ERRORCODE_AXISSTOPPING                   = 0x506,
    MC_ERRORCODE_AXISERRORSTOP                  = 0x507,
}MC_ErrorCode;

#endif
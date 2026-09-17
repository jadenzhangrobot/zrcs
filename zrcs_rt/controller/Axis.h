/*
 * @Author: zhangyongjing
 * @email: 649894200@qq.com
 * @Date: 2023-03-15 14:36:47
 * @LastEditTime: 2023-06-06 16:48:44
 * @Description: axis hardware abstraction
 */
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Servo.h"

// ── 全局作用域枚举（与全局头文件历史一致，全工程无前缀引用）──

/* Motion control error code */
typedef enum {
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
    MC_ERRORCODE_INVALIDSTATESTIPPING           = 0x32,   // Invalid state transition at Stopping
    MC_ERRORCODE_INVALIDSATATESTOP              = 0x33,   // Invalid state transition at ErrorStop
    MC_ERRORCODE_INVALIDSTATEDISABLE            = 0x34,   // Invalid state transition at Disabled
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
    MC_ERRORCODE_INVALID_DIRTCTION_POSITIVE     = 0x1F1, //正向移动不合法
    MC_ERRORCODE_INVALID_DIRTCTION_NEGATIVE     = 0x1F2, //负向移动不合法
    MC_ERRORCODE_MULTI_DRIVE_SYNC_ERROR         = 0x1F3, //多驱轴同步误差过大

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
} MC_ERROR_CODE;

/* Motion direction */
typedef enum {
    mcPositiveDirection = 1,
    mcShortestWay       = 2,
    mcNegativeDirection = 3,
    mcCurrentDirection  = 4,
} MC_DIRECTION;

/* Motion buffer mode */
typedef enum {
    /**
     * @brief Start FB immediately (default mode).
     * The next FB aborts an ongoing motion and the command
     * affects the axis immediately. The buffer is cleared.
     */
    mcAborting = 0,
    /**
     * @brief Start FB after current motion has finished.
     * The next FB affects the axis as soon as the previous movement is 'Done'.
     * There is no blending.
     */
    mcBuffered = 1,
    /**
     * @brief The velocity is blended with the lowest velocity of both FBs.
     */
    mcBlendingLow =
        2,  /// The velocity is blended with the lowest velocity of both FBs
    mcBlendingPrevious =
        3,  /// The velocity is blended with the velocity of the first FB
    mcBlendingNext =
        4,  /// The velocity is blended with velocity of the second FB
    mcBlendingHigh =
        5  /// The velocity is blended with highest velocity of both FBs
} MC_BUFFER_MODE;

/* Motion mode */
typedef enum {
    mcNoMoveType   = 0,
    mcMoveAbsolute = 1,
    mcMoveRelative = 2,
    mcMoveAdditive = 3,
    mcMoveVelocity = 4,
    mcHalt         = 5,
    mcStop         = 6
} MC_MOTION_MODE;

namespace ZrcsHardware {

/**
 * @brief Controller/Axis 运行时使用的轴参数。
 *
 * 该结构刻意和 zrcs::config::AxisConfigData 分开：config 命名空间描述文件数据，
 * 这里描述控制器运行时需要的形态，并可以保留 frequency 这类只属于控制器的字段。
 */
struct AxisPara {
    uint32_t axisId = 0;
    std::string axisName;
    std::vector<uint32_t> servoSlaveIds;
    double maxVel = 0.0;
    double maxAcc = 0.0;
    double maxJerk = 0.0;
    double posPositiveLimit = 0.0;
    double posNegativeLimit = 0.0;
    double maxPosDiff = 0.0;
    double lead = 0.0;
    double frequency = 0.0;
};

/// 轴级逻辑状态机（PLCOpen 风格），由周期循环聚合各 servo 状态推导。
class Axis
{
public:
    enum class AxisState
    {
        Disabled = 0,  /// 初始状态：未上电、无错误，不响应运动指令
        Standstill = 1,  /// 已上电、无错误、无运动指令执行中
        Homing = 2,  /// 回零中
        DiscreteMotion = 3,  /// 离散运动（到位停止）
        ContinuousMotion = 4,  /// 连续运动（速度模式）
        Stopping = 5,  /// 停止处理中
        ErrorStop = 6  /// 错误停机，最高优先级；需复位后才能恢复
    };

private:
    AxisPara* config_;
    std::vector<std::unique_ptr<Servo>> servo_;

    uint32_t axisId_ = 0;
    std::string axisName_ = "";
    double axisPos_ = 0;
    double axisVel_ = 0;
    double axisAcc_ = 0;
    double axisJerk_ = 0;
    double axisPosCmd_ = 0;
    double lastAxisPosCmd_ = 0;
    double axisVelCmd_ = 0;
    double lastAxisVelCmd_ = 0;
    double axisTorCmd_ = 0;
    AxisState axisState_ = AxisState::Disabled;
    MC_ERROR_CODE axisError_ = MC_ERRORCODE_GOOD;

    bool powerStatus_ = false;
    bool reset_ = false;
    bool enablePositive_ = true;
    bool enableNegative_ = true;

    double zeroOffset_ = 0;

public:
    Axis(uint32_t axisId, AxisPara* config);
    Axis(uint32_t axisId, uint32_t salveId, AxisPara* config);
    virtual ~Axis();

    void pushServo(std::unique_ptr<Servo> servo);
    void pushServo(std::unique_ptr<Servo> servo, const ServoPara& config);

    size_t servoCount() const;

    MC_ERROR_CODE setAxisId(uint32_t id);
    MC_ERROR_CODE setAxisName(std::string name);

    void setAxisPositionCmd(double axisPosCmd);
    void setAxisVelocityCmd(double axisVelCmd);
    void syncCmdHistory();

    /// 电机圈数 ↔ 轴用户单位，仅使用轴参数 lead（用户单位/圈）。
    double toUserUnit(double motorTurns) const;
    double toServoUnit(double axisUnit) const;

    bool cmdsProcessing(double frequency);
    void updateMotionCmdsToServo();
    void statusSync();

    double actualPos();
    double actualVel();
    double actualAcc();
    double actualPosCmd();
    double actualVelCmd();

    AxisState getAxisState(void);
    MC_ERROR_CODE setAxisState(AxisState setState);

    void cyclerun();
    bool resetError(void);

    bool powerOn();
    bool powerOff();
    bool isPowerOn() const { return powerStatus_; }
    void setModeOfOperation();
    void setModeOfOperation(Cia402Mode mode);

    MC_ERROR_CODE getAxisError();
    MC_ERROR_CODE servoErrorToAxisError(MC_SERVO_CODE error_id);

    double getMaxVelocity();
    double getMaxAcceleration();
    double getMaxJerk();
    double getLead() const;
    double getPositiveLimit() const;
    double getNegativeLimit() const;

    void setZeroOffset(double offset);
    double getZeroOffset() const;

    void setPosLimits(double posLimit, double negLimit);
    void setVelLimits(double maxVel, double maxAcc, double maxJerk);
};

} // namespace ZrcsHardware

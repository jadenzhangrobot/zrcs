#ifndef GLOBAL_H
#define GLOBAL_H
#include <cstdint>
#include <string>
typedef enum{
  SERVONOERROR                = 0,
  SERVOFIELDBUSINITERROR      = 1,
  SERVOPOWERERROR             = 2,
  SERVOPOWERINGONERROR        = 3,
  SERVOERRORWHENPOWEREDON     = 4,
  SERVOPOWERINGOFFERROR       = 5
}MC_SERVO_CODE;
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
}MC_ERROR_CODE;
typedef enum {
  mcServoControlModePosition = 0,
  mcServoControlModeVelocity = 1,
  mcServoControlModeTorque = 2,
} MC_SERVO_CONTROL_MODE;
/**
 * Axis states
 */
typedef enum {
  /**
   * The state ‘Disabled’ describes the initial state of the axis.
   * In this state the movement of the axis is not influenced by the FBs. Power
   * is off and there is no error
   * in the axis.
   * If the MC_Power FB is called with ‘Enable’=TRUE while being in ‘Disabled’,
   * the state changes to
   * ‘Standstill’. The axis feedback is operational before entering the state
   * ‘Standstill’.
   * Calling MC_Power with ‘Enable’=FALSE in any state except ‘ErrorStop’
   * transfers the axis to the
   * state ‘Disabled’, either directly or via any other state. Any on-going
   * motion commands on the axis are
   * aborted (‘CommandAborted’).
   */
  mcDisabled = 0,

  /**
   * Power is on, there is no error in the axis, and there are no motion
   * commands active on the axis.
   */
  mcStandstill = 1,
  mcHoming = 2,              /// Homing
  mcDiscreteMotion = 3,      /// DiscreteMotion
  mcContinuousMotion = 4,    /// ContinuousMotion
  mcSynchronizedMotion = 5,  /// SynchronizedMotion
  mcStopping = 6,            /// Stopping

  /**
   * ‘ErrorStop’ is valid as highest priority and applicable in case of an
   * error. The axis can have either
   * power enabled or disabled and can be changed via MC_Power. However, as long
   * as the error is pend-
   * ing the state remains ‘ErrorStop’.
   * The intention of the ‘ErrorStop’ state is that the axis goes to a stop, if
   * possible. There is no further
   * motion command accepted until a reset has been done from the ‘ErrorStop’
   * state.
   * The transition to ‘ErrorStop’ refers to errors from the axis and axis
   * control, and not from the Function
   * Block instances. These axis errors may also be reflected in the output of
   * the Function Blocks ‘FB
   * instances errors’.
   */
  mcErrorStop = 7,
} MC_AXIS_STATES;
/* Motion direction */
typedef enum {
  mcPositiveDirection = 1,
  mcShortestWay = 2,
  mcNegativeDirection = 3,
  mcCurrentDirection = 4,
} MC_DIRECTION;

/* Network error code */
typedef enum {
  mcNetworkGood = 0,
  mcNetworkConnectionBreak = 1,
} MC_NETWORK_ERROR_CODE;

/**
 * MC_BUFFER_MODE
 *
 */
typedef enum {
  /**
   * @brief Start FB immediately (default mode).
   * The next FB aborts an ongoing motion and the command
   * affects the axis immediately. The buffer is cleared.
   */
  mcAborting = 0,
  /**
   * @brief Start FB after current motion has finished.
   * The next FB affects the axis as soon as the previous movement is ‘Done’.
   * There is no blending.
   */
  mcBuffered = 1,
  /**
   * @brief The velocity is blended with the lowest velocity of both FBs.
   *
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

typedef enum {
  mcNoMoveType   = 0,
  mcMoveAbsolute = 1,
  mcMoveRelative = 2,
  mcMoveAdditive = 3,
  mcMoveVelocity = 4,
  mcHalt = 5,
  mcStop = 6
} MC_MOTION_MODE;


/**
 * @brief Represents the CiA 402 modes of operation (Object 6060h).
 * 
 * The underlying type is int8_t because the standard defines this object
 * as a SINT (Signed 8-bit integer).
 */
enum class Cia402Mode : uint8_t {
    NO_MODE_ASSIGNED = 0,
    PROFILE_POSITION = 1,
    // Value 2 is reserved
    PROFILE_VELOCITY = 3,
    PROFILE_TORQUE = 4,
    // Value 5 is reserved
    HOMING = 6,
    INTERPOLATED_POSITION = 7,
    CYCLIC_SYNCHRONOUS_POSITION = 8, // CSP
    CYCLIC_SYNCHRONOUS_VELOCITY = 9, // CSV
    CYCLIC_SYNCHRONOUS_TORQUE = 10,  // CST
    // Other values can be manufacturer-specific
};

#endif
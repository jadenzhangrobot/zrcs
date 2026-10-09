(* plc.st — PLC 最小演示逻辑（P0）
 *
 * 本文件同时包含程序和任务配置，只需维护这一份 ST。
 * CMake 按 BUILD_MODE 注入统一控制周期，再由 matiec 编译。
 * PlcInputNode/PlcOutputNode 读写 ZRCS__DEMO，PlcLogicNode 调用 config_run__，
 * 接入方式见 docs/matiec-rt-integration.md。
 *
 * 注意：IEC 数组下标为 1..N，生成 C 后为 0..N-1。
 *)
PROGRAM plc
VAR
    (* 移动阈值（单位 m，与控制器内部一致） *)
    MotionThreshold : LREAL := 0.001;
    i               : INT;
END_VAR

VAR_INPUT
    (* 机床 -> PLC：轴实际位置（m）与使能状态，对应 %I 定位输入 *)
    I_AxisPos     : ARRAY[1..8] OF LREAL;
    I_AxisEnabled : ARRAY[1..8] OF BOOL;
END_VAR

VAR_OUTPUT
    (* PLC -> 机床：逻辑/联锁输出，对应 %Q 定位输出 *)
    Q_Running   : BOOL;
    Q_ScanAlive : BOOL;
END_VAR

(* 任一已使能轴实际位置越过阈值 => 输出 Running 置位 *)
Q_Running := FALSE;
FOR i := 1 TO 8 DO
    IF I_AxisEnabled[i] AND (I_AxisPos[i] > MotionThreshold) THEN
        Q_Running := TRUE;
    END_IF;
END_FOR;

(* 每个扫描周期置位，作为扫描心跳（联调观测用） *)
Q_ScanAlive := TRUE;

END_PROGRAM

(* 任务周期来自 cmake/controller.cmake 的编译期配置：
 * realtime/simulation = 1ms，standard = 10ms；与 C++ 的周期宏保持一致。
 *
 * 命名约束（matiec 实测）：
 *  - IEC 标识符不含空格、大小写不敏感；CONFIGURATION/RESOURCE 名决定生成文件名。
 *  - RESOURCE 名不得与已声明的 POU 名相同：PROGRAM plc 之后再用 RESOURCE PLC，
 *    词法层会把 PLC 当成已声明的程序名，报 "unknown error in resource declaration"；
 *    且二者都会生成 <名>_init__（PLC_init__），C 链接也会冲突。
 *  - 实例符号 = <RESOURCE>__<PROGRAM 实例名>，即 ZRCS__DEMO；
 *    改名需同步改 zrcs_rt/CMakeLists.txt 与 zrcs_rt/plc/PlcProgram.h。 *)
CONFIGURATION zrcs_plc
    RESOURCE ZRCS ON ZRCS
        TASK Scantask(INTERVAL := T#@ZRCS_CYCLE_TIME_MS@ms, PRIORITY := 0);
        PROGRAM Demo WITH Scantask : plc;
    END_RESOURCE
END_CONFIGURATION

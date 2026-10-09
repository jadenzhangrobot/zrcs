# 在 RT 进程内兼容 MatIEC 编译 C 代码的集成方案

> 文档性质：接入说明与后续设计。最小 PLC 扫描已落地；完整 IO 映射、安全仲裁与 GUI 通道仍属于后续工作。
>
> 已确定的关键决策：
> - **PLC 定位 = 逻辑/联锁层**：只做 IO、联锁、使能、门控与低速点位；轨迹激活时轨迹层优先，PLC 不争抢位置指令。
> - **执行频率 = 与 RT 周期同频**：realtime/simulation 为 1 ms，standard 为 10 ms。

## 实现状态（2026-09-28 更新）

已落地并构建通过（P0 完整闭环），当前为**真实 MatIEC 编译器产出**，不再使用桩：

- **matiec 编译器已构建**：`3rdParty/MatIEC/iec2c.exe`（在 MSYS2 内装 autotools 后 `autoreconf -i && ./configure && make` 编译）。注意 `iec2c.exe` 是 **MSYS2 宿主**工具，运行时依赖 `/usr/bin` 的 `msys-2.0.dll` 等，CMake 生成步骤需 PATH 含 MSYS 的 `/usr/bin`（UCRT64 构建环境即满足）。
- **生成物进 build 树（Qt 式）**：CMake `add_custom_command` 跑 matiec 把 `plc.st` 生成到 `build/zrcs_rt/plc_generated/`；ST 源码随工程放 `config/5axis/program/plc/plc.st`，宿主源码位于 `zrcs_rt/plc/`。真实 matiec 产物（`zrcs_plc.c` / `ZRCS.c` / POUS 等）不入库。生成文件名由 ST 中的 CONFIGURATION / RESOURCE 名决定。
- **直接接入生成接口**：三个节点通过 `PlcProgram.h` 的 C 链接声明共享生成实例 `ZRCS__DEMO`（= `<RESOURCE>__<PROGRAM 实例名>`）；该头文件只引入生成接口，不提供中间数据副本或桥接函数。生成源码仍以 C 编译。
- **三个周期节点**：`PlcInputNode(INPUT, 100)` 采集轴反馈并写入 PLC 输入；`PlcLogicNode(OUTPUT, 100)` 推进时间并扫描；`PlcOutputNode(OUTPUT, 110)` 读取扫描结果并处理输出。三个 `run()` 均带 `TaskScheduling::RUN` 门控，PLC 实例只由逻辑节点在注册阶段初始化。
- **输入与时间**：位置直接使用 `LREAL`，使能读取轴真实上电反馈并排除故障；时间按 `ZRCS_CYCLE_TIME_MS` 积分。仅维护一份 `plc.st`，其中同时包含 PROGRAM 和 CONFIGURATION。`cmake/controller.cmake` 按 BUILD_MODE 在编译期选择周期，既传给 C++ 周期宏，也替换 ST 中的 `@ZRCS_CYCLE_TIME_MS@`；不对 ST 运行 C 预处理器。当前输出仅做边沿日志，尚未驱动物理 IO。
- **构建依赖**：C++ 节点编译前先生成 `POUS.h` 等产物；未找到 MatIEC 时在 CMake 配置阶段明确报错。编译器路径可通过 `ZRCS_MATIEC_EXE` 指定。

以下涉及通用定位变量、映射表和 GUI 的章节是后续设计；当前演示程序使用普通 `VAR_INPUT/VAR_OUTPUT` 字段，并未声明 `AT %I/%Q`。

---

## 1. 目标与范围

### 1.1 目标

让用户在《编程器/仿真器》里用 IEC 61131-3（结构文本 ST 为主）编写的逻辑程序，能在 zrcs 实时侧以确定性方式运行：

- PLC 扫描作为一个**周期节点**嵌进现有控制循环，周期由 `ZRCS_CYCLE_TIME_MS` 统一确定，不新增实时线程；
- PLC 通过**定位变量（AT `%I`/`%Q`）**与轴状态、IO 交换数据；
- 遵循 [realtime skill](../.claude/skills/realtime/skill.md) 的实时约束（循环内无分配、无异常、无阻塞调用、确定性有界）。

### 1.2 范围

| 在范围内 | 不在范围内 |
|---|---|
| MatIEC C 生成物的 CMake 集成 | 修改现有运动学/伺服代码 |
| `PlcLogicNode` 周期节点与数据桥 | 在主线轨迹执行路径里插入 PLC |
| `%I/%Q` ↔ 轴/IO/状态的映射与安全仲裁 | GUI 侧变量在线监控（后续再议） |
| 与 RT 同频的时序与时间注入 | ST→C 的浏览器侧编辑/仿真器集成 |
| 构建管线（ST 源码 → C 生成） | |

### 1.3 术语

- **MatIEC**：IEC 61131-3 开源编译器，把 ST/FBD/LD 编译为 C。
- **POU**：程序组织单元（Program / Function Block / Function）。
- **扫描（scan）**：`config_run__` 一次完整调用，对应一个 PLC 周期。
- **定位变量**：`AT %I*`/`AT %Q*`/`AT %M*` 声明的全局变量，是 PLC 与外部世界的交换界面。

---

## 2. MatIEC 产物形态

MatIEC 把一段 ST 工程编译成一组**纯 C 文件**，不依赖 C++ 运行时：

```
<project>/
├─ config.c / config.h         # 顶层配置：config_init__()、config_run__()、定位变量
├─ <POU>__.c /.h               # 各 POU 的 __init / __run
├─ (若干 FB 实例).c
└─ 运行时库：iec_std_lib.h / iec_std_lib.c   # 随 MatIEC 分发
```

宿主负责的调用契约极其简单：

```c
/* 一次 */
config_init__();

/* 每个扫描周期 */
__CURRENT_TIME = 推进后的单调时间;   /* IEC_TIMESPEC，驱动 TON/TONR/边缘检测 */
config_run__(tick++);
```

三个对集成最重要的机制：

1. **执行模型**：一切逻辑都在 `config_run__` 里按声明顺序同步执行完，一次调用 = 一个 PLC 周期。没有并发、没有事件循环，适合按 RT 控制周期轮询。
2. **时间 = 全局 `__CURRENT_TIME`**（`IEC_TIMESPEC`，含秒 + 纳秒）。定时器使用该时间，**由宿主每次扫描按控制周期累加**。当前采用周期积分，不读取系统时钟；不能用 `time()`，避免校时/跳变导致定时异常。
3. **定位变量 = extern 全局**：`AT %I*`/`%Q*` 被编译成可被宿主 `extern` 引用的符号（符号命名随版本而异，最终以生成产物为准，见 §6.1）。这就是 PLC ↔ 机床的数据交换壳。

### 2.1 为什么它适合实时侧

- `config_run__` 内**无动态内存分配、无异常、无阻塞系统调用**——与 zrcs 实时规范完全一致，这是「兼容可行性」的根本前提。
- 潜在代价：顶层 `config_` 是聚合了全部变量的**大结构体**，反复整体赋值会变成整块 memcpy。设计上用「指针/引用直连定位变量」规避大拷贝（见 §6.2）。

---

## 3. 总体架构

PLC 输入、扫描、输出分别作为三个 `PeriodicNode`，嵌进 [NodeManager.cpp](../zrcs_rt/system/NodeManager.cpp) 的 `real_task` 回调。输入采样位于命令执行前，扫描与输出位于命令执行后，全部在同一 RT 线程内按序执行。

```
RT 回调（周期由 ZRCS_CYCLE_TIME_MS 确定）
  receiveData()
    → INPUT 阶段
        PlcInputNode::run()     [order=100] RT 变量 → PLC 输入
    → RUN 命令/轨迹处理
    → OUTPUT 阶段
        ContinuousJog          [order=80]
        PlcLogicNode::run()     [order=100] 推进时间 → config_run__()
        PlcOutputNode::run()    [order=110] PLC 输出 → RT 侧处理
    → 状态回写 + sendData()
```

**为什么不开新线程**：PLC 扫描是非阻塞的同步计算，放进现有单线程即可。开线程只会带来锁、跨线程共享 `%I/%Q`、优先级反转三类问题，与实时规范冲突。

---

## 4. 挂载点：输入、扫描、输出节点

### 4.1 类设计

三个节点均派生 [PeriodicNode](../zrcs_rt/system/node/BaseNodeInterface.h)，使用 `REGISTER_PERIODIC` 静态注册，由 `NodeFactory` 收编并排序、`NodeManager` 调度。

```
zrcs_rt/plc/PlcInputNode.h / .cpp   // RT 变量 → PLC 输入
zrcs_rt/plc/PlcLogicNode.h / .cpp
zrcs_rt/plc/PlcOutputNode.h / .cpp  // PLC 输出 → RT 变量/IO
zrcs_rt/plc/PlcProgram.h           // 生成接口声明
```

```
class PlcLogicNode : public zrcsSystem::PeriodicNode {
public:
    PlcLogicNode();
    void init() override;     // 空操作，注册阶段已完成初始化
    void run()  override;     // 时间→config_run__，映射交给输入/输出节点
protected:
    void onRegistered() override;         // RT 启动前 config_init__()
private:
    uint64_t tick_ = 0;
    int64_t scanTimeNs_ = 0;
};
```

要点：

- **`onRegistered()`** 在 RT 任务启动前执行，清零时间并调用一次 `config_init__()`。
- **`init()`** 不重复初始化 PLC，避免在首个 RT 周期重置状态。
- **输入节点 `run()`** 在 INPUT 阶段采样轴位置和真实上电状态，通过 `__SET_VAR` 写入实例；保留 LREAL 精度、FORCE 和空轴槽处理。
- **逻辑节点 `run()`** 只推进时间并调用 `config_run__()`。
- **输出节点 `run()`** 在逻辑节点后通过 `__GET_VAR` 读取本拍结果；`lastRunning_` 由输出节点持有，当前用于边沿日志，物理 IO 写入仍待映射。

### 4.2 调度器的接缝（无需改 `NodeManager.cpp`）

OUTPUT 周期节点由 `NodeManager` 的 `factory_.outputPeriodics` 调度。当前命令完成分支会提前退出 `switch`，该拍不会执行 OUTPUT 阶段；独立桥接层的移除不改变此既有调度行为，严格同频扫描仍需修复这个调度问题。

需要补的最小接缝只有一处：`TaskScheduling` 对 PLC 的「使能门控」，由 `run()` 内部读取 `shm()->taskSched` 自行判断（见 §7），保持调度器零改动。

### 4.3 注册接线

- 输入节点：`REGISTER_PERIODIC(PlcInputNode, INPUT, 100);`
- 逻辑节点：`REGISTER_PERIODIC(PlcLogicNode, OUTPUT, 100);`
- 输出节点：`REGISTER_PERIODIC(PlcOutputNode, OUTPUT, 110);`
- `zrcs_rt/plc/*.cpp` 已由 [zrcs_rt/CMakeLists.txt](../zrcs_rt/CMakeLists.txt) 的 `GLOB_RECURSE CONFIGURE_DEPENDS` 覆盖，新增节点自动加入构建。
- 参照 `MujocoIdentPub`：新增节点类 + 注册宏即可被工厂拾取，**不需要**进 [CmdHead.h](zrcs_rt/command/CmdHead.h)（那是命令命令头，OutputNode 不走 CmdId 路由）。

---

## 5. 数据桥定义（%I / %Q ↔ 轴 / IO）

### 5.1 设计原则

- **单位**：控制器/规划内部统用 **m**（见项目记忆 `project_units_meters`），PLC 侧 `IEC_REAL` 同样是 float 米，两侧统一，桥内不做单位换算；若有 mm 源，在进桥前转换。
- **Input 桥（机床→PLC）**：把轴反馈与状态写入 `%I`。**只读**轴上值，绝不给轴写。
- **Output 桥（PLC→机床）**：把 `%Q` 用于 IO 输出 + 门控 + 低速点位。

### 5.2 映射表（建议雏形，实际以 axis.xml/IO 配置驱动）

| 定位变量 | 类型 | 方向 | 数据源/去向 |
|---|---|---|---|
| `%IW0.0`~`N` | IEC_REAL | IN | `axis[i]->actualPos()`（m） |
| `%IW1.0`~`N` | IEC_REAL | IN | `axis[i]->actualVel()` |
| `%IW2.i` | IEC_BOOL | IN | `axis[i]->getAxisState()` 使能/到位派生 |
| `%IW3.x` | IEC_BOOL | IN | IO 输入位（机械限位、按钮等） |
| `%QW0.0`~`N` | IEC_REAL | OUT | IO 模拟输出 / 低速点位目标（仅仲裁放行时） |
| `%QW1.x` | IEC_BOOL | OUT | IO 输出（气缸、阀、指示灯） |

> 布尔定位位与实数定位字的**地址排布由 MatIEC 的字节/位分配规则决定**，映射表应依据生成产物的符号名与偏移生成，而不是硬编码猜测（见 §6.1）。

### 5.3 映射表的构建方式

推荐**配置驱动**：新增 `plcIoMapping.xml`（或在 `axis.xml` 旁定义），声明「哪个 PLC 变量 ↔ 哪个轴/IO」；`onRegistered()` 解析后填充 `PlcIoMapping`（固定大小数组，`std::array`，无运行时分配），RT 期只做 O(1) 下标读写。

不允许在 `run()` 里解析 XML 或分配映射——那属于实时路径。

---

## 6. 已编译 C 的接入细节

### 6.1 当前生成接口的直接调用

三个节点的 `.cpp` 通过 `PlcProgram.h` 引用真实生成接口，各节点的类头文件不暴露 MatIEC 类型。MatIEC 的 C 标准依赖先在全局包含，再用命名空间隔离其 `tm/BOOL` 等类型名：

```cpp
namespace matiec {
extern "C" {
#include "POUS.h"
extern PLC ZRCS__DEMO;
void config_init__(void);
void config_run__(unsigned long tick);
extern TIME __CURRENT_TIME; // 唯一定义位于 PlcLogicNode.cpp
}
}
```

在 `PlcInputNode::run()` 中写 `__SET_VAR(matiec::ZRCS__DEMO., I_AXISPOS, .table[i], position)`，在 `PlcOutputNode::run()` 中读 `__GET_VAR(matiec::ZRCS__DEMO.Q_RUNNING, )`，保留 MatIEC 的 FORCE 语义。新增 PLC 变量时，在 ST 中声明输入/输出，再到相应节点添加映射。命名空间只隔离 C++ 类型，生成 C 的链接符号不变。数组容量从生成类型推导。ST 配置名、资源名、程序名或字段变更时，需同步更新接口声明或对应节点的字段引用；生成文件本身无需手动修改。

### 6.2 后续定位变量的符号绑定

MatIEC 定位变量在生成 C 里的**符号名**（可能形如 `__IX0_0`、`__QW0`，或经 `config_loc.c` 的宏/数组展开）必须在集成时核对真实产物。建议在 `PlcIoMapping` 里用 `extern` 声明 + 编译期断言固定下来，例如：

```c
/* 以 config.c 实际导出的符号为准，勿照抄本示例命名 */
extern IEC_REAL __IW0[ N ];
extern IEC_BOOL __QX0[ M ];
```

并在 CMake 里把生成目录加为 include 路径，`#include "config.h"` 直接引用。

> ⚠️ 集成首批任务里应有一项：**用一个小 ST 工程编译出产物，把定位变量符号清单固化进本文档/代码**，消除对命名规则的猜测。

### 6.3 规避大结构体拷贝

顶层 `config_` 聚合全部变量，整体赋值代价高。应对措施：

- 需要交换的定位变量，通过**指针/引用直连**其全局符号读写，而不是 `memcpy(config_)`；
- 若无法避免拷贝，则量测最差执行时间并确保在 1ms 预算内（见 §8）。
- 触发扫描用「最低必要地」更新 `__CURRENT_TIME`，避免无谓写整个时间结构。

### 6.4 时间注入

```c
/* 每周期在 config_run__ 之前 */
__CURRENT_TIME.tv_sec  = nowSec;          /* 取自 RT 单调钟/周期计 */
__CURRENT_TIME.tv_nsec = nowNsec;
```

- 来源必须是确定性时钟；仓库当前用 RT 循环计数推进即可（`cycletime` 单位 ms，见 [MoveL.cpp](zrcs_rt/command/MoveL.cpp) 的 `cycletime * 0.001`）。
- 保证单调不后退（防止 `time()` 校时造成 TON 复位）。
- `PlcLogicNode::onRegistered()` 在 RT 线程启动前校验生成的 `common_ticktime__`（ns）等于 `ZRCS_CYCLE_TIME_MS * 1000000`；不一致时拒绝启动，提示按当前 BUILD_MODE 重新生成 PLC，防止误链接旧模式产物。
- 以首次扫描建立定时起点后，1 秒的 TON 在 standard 下经过 100 个 10 ms 间隔到期，在 realtime/simulation 下经过 1000 个 1 ms 间隔到期；STOP 等非 RUN 状态仍冻结 PLC 时间。

---

## 7. 安全集成与仲裁（关键）

### 7.1 与轨迹层的写入权仲裁（决策：逻辑/联锁层）

两条写轴指令的路径必须互斥，否则互相覆盖：

| 场景 | 位置指令归属 |
|---|---|
| `TaskScheduling != RUN`（STOP/ERROR/SHUTDOWN/IDLE） | 都不写；轴线由既有逻辑冻结/断电 |
| `RUN` 且无活动 `cmdNode_` | 允许 PLC 低速点位（可选，通过配置开关） |
| `RUN` 且有活动轨迹命令 | **轨迹优先**；PLC 只做门控/联锁/IO，不写 `setAxisPositionCmd` |

实现：`run()` 内读取 `this->getCmdStatus()` 附近的活动命令状态（或 `NodeManager` 暴露的 `cmdNode_` 空否）来决定 Output 桥是否写点。

### 7.2 与 TaskScheduling 的门控

- `run()` 首行读取 `shm()->taskSched`；仅 `RUN` 时执行 ③`config_run__` 与输出，其余状态**冻结输出**（不写 `%Q` 到机床 / 不复位错误）。
- 不主动复位轴错误码；恢复流程仍走既有 `RESET`，避免 PLC 绕开安全机制。
- 现有 `hasAxisFault()/enterErrorStateFromAxisFault` 逻辑（[NodeManager.cpp](zrcs_rt/system/NodeManager.cpp)）保持对 PLC 有效：PLC 输出不能把已 ErrorStop 的电机重新驱动。

### 7.3 使能安全

- PLC 触发轴使能须经既有 `axis->setAxisState()` 的校验返回码，失败即拒绝并上报错误码，不静默忽略。
- 为 PLC 单独设一个 `enable` 门信号（位于 `%Q` 或配置），使能 = TaskScheduling 允许 ∧ PLC 允许。

---

## 8. 实时性合规清单

对照 [realtime skill](../.claude/skills/realtime/skill.md)：

| 规范条款 | PLC 侧落实情况 |
|---|---|
| 循环内禁止异常 | `config_run__` 不抛异常；宿主桥代码用返回值/错误码，不用 try/catch |
| 循环内禁止动态分配 | `onRegistered()` 完成全部映射预分配；`run()` 零分配 |
| 循环内禁止阻塞系统调用 | `config_run__` 纯计算；桥只做内存读写 |
| 确定性、有界 | 需实测最差 `config_run__` 耗时 + 桥开销，留出 1ms 预算 |
| 定时/时间 | `__CURRENT_TIME` 用单调钟推进，保证确定性 |

**预算验收**：用一个「最坏规模」的 ST 工程，量测 `run()`（桥 + 扫描 + 时间）的 P99 耗时，确认 < `X` ms（建议控制周期 1ms 内留 ≥30% 余量）。若超标，退化到整倍变频（把「每 N 周期跑一次」做成配置项，勿做进主线逻辑后端）。

---

## 9. CMake 与构建管线

遵循 [3rdparty-lib skill](../.claude/skills/3rdparty-lib/skill.md) 与 [windows-build skill](../.claude/skills/windows-build/skill.md)：

1. **MatIEC 运行时库**（`iec_std_lib.h` 等，纯头文件、跨工程复用）→ 放 `3rdParty/MatIEC/lib/C/`；编入 `zrcsrt` include 路径，在 `cmake/3rdParty.cmake` 登记。
2. **matiec 编译器**：`3rdParty/MatIEC/iec2c.exe`，MSYS2 内 `pacman -S autoconf automake` 后 `autoreconf -i && ./configure && make` 构建（需 flex/bison/g++）。
3. **ST 源码**（受版本控制）= 用户的逻辑，随工程放 `config/<active>/program/plc/*.st`（详见 §10 目录约定）。
4. **生成 ST/C** = 构建产物，放 **build 树** `build/zrcs_rt/plc_generated/`（不入库）。`configure_file(plc.st ... @ONLY)` 直接替换单个 ST 内的 `@ZRCS_CYCLE_TIME_MS@`，无需单独的 `.st.in` 配置模板；`add_custom_command` 再运行 MatIEC。`configure_file` 自动跟踪源文件，原始程序修改及 BUILD_MODE 切换都会更新生成物。源文件含构建期占位符，手动运行 MatIEC 时应使用构建目录中替换完成的 ST。
5. **宿主节点**（版本受控，C++）：输入、逻辑、输出三个节点通过 `PlcProgram.h` 引入生成的 `POUS.h`，共同访问同一个实例。逻辑节点独占 `config_init__/config_run__` 调用。`zrcs_plc_generated` 目标确保首次并行构建时先生成头文件。
6. 统一用 UCRT64 工具链编译这些 C 文件（[windows-build](../.claude/skills/windows-build/skill.md) 强制要求；构建必须走 `cmake --build`，勿直接 `mingw32-make`，见项目记忆）。注意 `iec2c.exe` 是 MSYS2 宿主工具，运行步骤需 PATH 含 MSYS 的 `/usr/bin`。

---

## 10. 文件清单与改动点（汇总）

| 操作 | 路径 | 说明 |
|---|---|---|
| 新增 | `zrcs_rt/plc/PlcInputNode.h/.cpp` | INPUT 周期节点：RT 变量 → PLC 输入 |
| 新增 | `zrcs_rt/plc/PlcLogicNode.h/.cpp` | OUTPUT 周期节点：初始化、时间 + 扫描 |
| 新增 | `zrcs_rt/plc/PlcOutputNode.h/.cpp` | OUTPUT 周期节点：扫描后读取、处理 PLC 输出 |
| 新增 | `zrcs_rt/plc/PlcProgram.h` | 生成接口声明，无中间变量副本 |
| 新增 | `zrcs_rt/plc/PlcIoMapping.h/.cpp` | `%I/%Q` ↔ 轴/IO 固定映射表 |
| 新增 | `config/<active>/program/plc/*.st` | PLC 逻辑源码（版本受控，随工程） |
| 新增 | `config/<active>/program/plc/` 等 | 工程 program 按功能分类目录（见 §10 目录约定） |
| 新增 | `build/zrcs_rt/plc_generated/` | 构建生成的 C（build 树，不入库） |
| 直接引用 | 三个节点 → `PlcProgram.h` → `plc_generated/POUS.h` | C 链接、实例输入输出与时间注入，不设独立桥接文件 |
| 新增 | `docs/plc_io_mapping.md` 或 `plcIoMapping.xml` | 映射配置说明/文件 |
| 新增(第三方) | `3rdParty/MatIEC/` | `iec_std_lib.*` 运行时 + `iec2c.exe` 编译器 |
| 修改 | `zrcs_rt/CMakeLists.txt` | 加 `plc` GLOB、matiec 生成自定义命令、include 与链接运行时 |
| 修改 | `cmake/3rdParty.cmake` | 登记 MatIEC |
| 修改 | `zrcs_rt/command/CmdHead.h` | 不需要（OutputNode 不走命令路由）；仅当新增 CmdId 命令时 |
| 修改 | `.gitignore` | （可选）忽略 `build/zrcs_rt/plc_generated/`（build 树本身已忽略） |

**不修改**：`NodeManager.cpp` 调度器、`Controller`、轴/伺服实现、`test/`（遵循记忆 `dont-touch-tests`，落地后只列出对测试的影响，不改动）。

---

## 11. 测试与验证

- **单元**：`PlcIoMapping` 的变量↔偏移绑定（纯表驱动，可单测）；时间推进的单调性。
- **仿真集成（SIMULATION 模式）**：在 MuJoCo/虚拟轴下，让 PLC 做使能→门控→低速点位，验证 `%Q`→`setAxisVelocityCmd` 生效、错误状态下降级。
- **时序**：最坏规模下 `run()` 耗时基准。
- **回归**：既有命令（MoveL 等）在 PLC 接入后行为不变（仲裁放行时轨迹优先）。

---

## 12. 分阶段落地计划（供批准后执行）

1. **P0 骨架**：CMake 集成 + `PlcLogicNode` 空壳 + 简单 ST 工程（一个 TON + 一个布尔门），验证能随 1ms 循环以确定性跑通，量测预算。
2. **P1 数据桥**：`PlcIoMapping` + `%I/%Q` 到轴状态/IO 的完整映射；用仿真验证。
3. **P2 安全仲裁**：与 `TrajectoryCmd` 的写入权、与 `TaskScheduling` 的门控；覆盖 STOP/ERROR 冻结。
4. **P3（后续）**：GUI 侧 PLC 变量在线监控、参数下发、离线仿真器集成。

---

## 13. 风险与未决项

- **定位变量符号名未固化**：需先跑一个最小 ST 工程确认（§6.1），这是最大的不确定点。
- **`config_` 大结构体性能**：按 §6.2 规避，若 ST 工程很大仍需量测。
- **PLC 直接驱动使能/点位的安全边界**：默认收紧（逻辑层 + 仲裁），如后续开放直控需评审。
- **MatIEC 版本锁定**：`3rdParty/MatIEC` 应固定版本 tag，避免生成物接口漂移。
- **布尔位寻址**：`%IX0.0` 这类位寻址在不同版本排布规则可能不同，映射生成需与具体版本对齐。
```

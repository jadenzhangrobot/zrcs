---
name: realtime
description: zrcs 实时程序编程规范。编写或修改 zrcsRt 目录下的实时控制循环代码时使用，涵盖内存、线程、错误处理、通信和代码风格。
---

# zrcs 实时程序编程规范

本规范适用于 `zrcsRt/` 目录下所有运行在实时线程（Xenomai / PREEMPT_RT）中的代码。实时代码的首要目标是 **确定性**（determinism）——每个控制周期的执行时间必须可预测且有界。

## 1. 实时线程中禁止的操作

以下操作会引入不确定延迟，**严禁**在实时循环（`run()`、`sendData()`、`receiveData()` 等周期性调用的函数）中使用：

### 1.1 禁止异常处理

```cpp
// 禁止 — try/catch 的栈展开和 RTTI 引入不确定延迟
try {
    doWork();
} catch (...) { }

// 正确 — 使用返回值或错误码
if (!doWork()) {
    axisError_ = MC_ERRORCODE_XXX;
    return false;
}
```

**原因**：C++ 异常的实现依赖栈展开（stack unwinding）和运行时类型信息（RTTI），这些操作的耗时不可预测，可能导致控制周期超时（jitter）。

### 1.2 禁止动态内存分配

```cpp
// 禁止 — new/delete/malloc/free 涉及内核页面管理，可能触发缺页中断
auto* p = new MyClass();
std::vector<int> v;       // push_back 时可能 realloc
std::string s = "hello";  // 短字符串优化之外会 malloc
std::map<int, int> m;     // 红黑树节点逐个 new

// 正确 — 使用预分配或栈上固定大小容器
char buf[192];
std::array<double, AXISMAXCOUNT> arr;
```

**原因**：`malloc/new` 可能触发操作系统的页面分配、缺页中断（page fault）或内存碎片整理，这些操作耗时从几微秒到几毫秒不等，对于 1ms 的控制周期是不可接受的。

### 1.3 禁止阻塞式系统调用

```cpp
// 禁止 — 文件 I/O、网络 I/O、阻塞式互斥锁
FILE* f = fopen("log.txt", "w");
std::mutex mtx; mtx.lock();       // 可能被低优先级线程持有导致优先级反转
std::cout << "debug" << std::endl; // 涉及 I/O 和内存分配
sleep(1);                          // 主动放弃 CPU

// 正确 — 使用无锁队列异步传递数据到非实时线程
g_logQueue->push(entry);  // SPSC 无锁环形队列，O(1) 确定性
```

**原因**：阻塞调用会使实时线程挂起，等待内核调度，期间控制周期被错过。互斥锁还可能引发优先级反转（priority inversion）。

### 1.4 禁止使用 STL 带有隐式分配的容器

实时路径中不得使用 `std::vector`、`std::map`、`std::unordered_map`、`std::string`、`std::list`、`std::deque` 等可能在运行时动态分配内存的容器。

允许使用的容器：
- `std::array<T, N>` — 编译期固定大小，栈上分配
- C 风格数组 `T arr[N]`
- 项目自定义的 `SPSCRingBuffer<T, N>` — 固定大小无锁队列

## 2. 实时线程初始化规范

以下操作**必须**在实时线程启动前（初始化阶段）完成：

### 2.1 内存锁定

```cpp
// 防止实时线程的内存页被交换到磁盘
mlockall(MCL_CURRENT | MCL_FUTURE);
```

### 2.2 栈预分配

```cpp
// 设置实时线程栈大小，避免运行时栈扩展触发缺页
pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
```

### 2.3 调度策略与优先级

```cpp
// 使用 SCHED_FIFO 实时调度策略，最高优先级
pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
param.sched_priority = 99;
pthread_attr_setschedparam(&attr, &param);
pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
```

### 2.4 CPU 亲和性（Xenomai）

```cpp
// 将实时任务绑定到专用 CPU 核心，减少缓存失效
cpu_set_t mask;
CPU_ZERO(&mask);
CPU_SET(cpu_id, &mask);
rt_task_set_affinity(&task_desc, &mask);
```

## 3. 错误处理规范

### 3.1 使用错误码

所有实时函数通过返回值或错误码成员变量报告错误：

```cpp
// 标准模式：返回 bool，错误码写入成员变量
bool Axis::cmdsProcessing(double frequency)
{
    if (std::abs(vel_cmd) > config_->maxVel) {
        axisError_ = MC_ERRORCODE_CMDVELOVERLIMIT;
        return false;
    }
    return true;
}
```

### 3.2 错误状态传播

当检测到错误时，将轴状态设为 `mcErrorStop`，停止后续指令下发：

```cpp
if (!it->cmdsProcessing(1000.0 / cycletime)) {
    it->setAxisState(mcErrorStop);
    continue;  // 跳过本轴指令下发
}
it->updateMotionCmdsToServo();
```

### 3.3 错误码定义

使用 `MC_ERROR_CODE` 枚举，按类别分段编号。伺服错误通过偏移映射：

```cpp
MC_ERROR_CODE servoErrorToAxisError(MC_SERVO_CODE error_id) {
    return static_cast<MC_ERROR_CODE>(0x60 + error_id);
}
```

## 4. RT 与 NRT 通信规范

实时线程（RT）与非实时线程（NRT）之间的数据交换**必须**使用无锁机制：

### 4.1 SPSC 无锁环形队列

```cpp
// 命令通道：NRT -> RT
SPSCRingBuffer<Command, COMMAND_BUFFER_SIZE> commandQueue;

// 状态通道：RT -> NRT
SPSCRingBuffer<std::array<double, AXISMAXCOUNT>, STATUS_BUFFER_SIZE> statusQueue;

// 日志通道：RT -> NRT
SPSCRingBuffer<RtLogEntry, LOG_BUFFER_SIZE> logQueue;
```

- 容量必须为 2 的幂（位掩码替代取模运算）
- 使用 `std::memory_order_acquire/release` 保证跨核可见性
- 队列满时静默丢弃，不阻塞

### 4.2 原子变量

标量状态使用 `std::atomic<T>` 传递，**必须**确认为 lock-free：

```cpp
std::atomic<uint64_t> heartBeat;
std::atomic<double> overrideRatio{1.0};

// 编译期断言：确保 atomic 操作是真正的硬件原子操作
static_assert(ATOMIC_INT_LOCK_FREE == 2,
              "atomic<int> must be lock-free for cross-process safety");
```

### 4.3 共享内存结构

所有跨进程共享数据集中在 `SharedBlock` 结构体中，通过 `ShmAccessor` 类型安全访问：

```cpp
ShmAccessor shm() { return ShmAccessor(rtProcess_->sharedBlock()); }
auto& queue = shm().cmdQueue();
```

## 5. 实时日志规范

### 5.1 使用项目日志宏

```cpp
INFO_PRINT("msg: %d\n", value);
WARN_PRINT("warning: %.4f\n", val);
ERROR_PRINT("error: code=%d\n", code);
```

### 5.2 日志实现原理

- 栈上格式化（`char buf[192]`），无动态分配
- 通过 SPSC 队列异步传递到 NRT 线程
- 队列满时静默丢弃，保证 RT 线程不阻塞
- Xenomai 环境下使用 `rt_printf` 替代 `printf`

### 5.3 禁止在 RT 中使用的日志方式

```cpp
// 禁止 — 这些都会触发系统调用或内存分配
std::cout << "debug" << std::endl;
printf("debug\n");  // 非 Xenomai 环境下的 printf 也不安全
fprintf(stderr, ...);
spdlog::info(...);  // 第三方日志库通常不是 RT-safe 的
```

## 6. 控制循环结构

### 6.1 标准周期流程

```
receiveData()          ← 从硬件总线接收伺服反馈
  ├─ hardwareBus_->receive()
  ├─ axis->statusSync()      ← 更新轴的实际位置/速度/加速度
  └─ axis->cyclerun()        ← 伺服周期处理

[节点调度器执行 CmdNode/OutputNode/InputNode 的 run()]

sendData()             ← 向伺服发送指令
  ├─ axis->cmdsProcessing()  ← 限位/速度/加速度检查
  └─ axis->updateMotionCmdsToServo()  ← 下发位置/速度指令
  └─ hardwareBus_->send()
```

### 6.2 周期时间

- 实时模式（`REALTIME` 宏定义）：`cycletime = 1`（ms）
- 仿真模式：`cycletime = 10`（ms）
- 频率计算：`frequency = 1000.0 / cycletime`

## 7. 代码风格

### 7.1 命名规范

| 类别 | 风格 | 示例 |
|------|------|------|
| 类名 | PascalCase | `Axis`, `TrajectoryCmd`, `NodeManager` |
| 成员变量 | camelCase + 尾部下划线 | `axisPosCmd_`, `axisState_`, `config_` |
| 局部变量 | camelCase | `rawPosCmd`, `velCmd` |
| 函数/方法 | camelCase | `cmdsProcessing()`, `statusSync()` |
| 常量/宏 | UPPER_SNAKE_CASE | `AXISMAXCOUNT`, `cycletime`, `MC_ERRORCODE_GOOD` |
| 枚举值 | camelCase 前缀 | `mcStandstill`, `mcErrorStop` |
| 命名空间 | PascalCase 或 camelCase | `ZrcsHardware`, `zrcsSystem` |

### 7.2 头文件

- 优先使用 `#pragma once`，旧代码中可见 `#ifndef` 守卫
- include 顺序：项目头文件 → 第三方库 → 标准库

### 7.3 数值类型

- 位置/速度/加速度等物理量使用 `double`
- 编码器原始值使用 `int32_t`
- 数组索引和大小使用 `size_t` 或 `uint32_t`

## 8. 运动命令开发规范

### 8.1 继承 TrajectoryCmd

所有基于轨迹规划的运动命令继承 `TrajectoryCmd`，实现以下虚函数：

```cpp
class MyMove : public TrajectoryCmd {
protected:
    bool initTrajectory() override;      // 设置 Ruckig 参数
    void applyDeltaTime(double dt) override; // 设置 otg.delta_time
    Result updateTrajectory() override;  // 调用 otg.update()
    void applyOutput() override;         // 写入轴位置指令
    void passOutputToInput() override;   // output.pass_to_input(input)
};
```

### 8.2 Ruckig 参数设置

```cpp
bool initTrajectory() override {
    // 从轴配置读取限制
    input_->max_velocity[i] = controller_->axiss[id]->getMaxVelocity();
    input_->max_acceleration[i] = controller_->axiss[id]->getMaxAcceleration();
    input_->max_jerk[i] = controller_->axiss[id]->getMaxJerk();
    
    // 设置当前位置和目标位置
    input_->current_position[i] = controller_->axiss[id]->actualPos();
    input_->target_position[i] = targetPos;
    return true;
}
```

### 8.3 位置指令下发

```cpp
void applyOutput() override {
    controller_->axiss[id]->setAxisPositionCmd(output_->new_position[i]);
}
```

## 9. 检查清单

在提交实时代码前，确认以下事项：

- [ ] 无 `try`/`catch`/`throw`
- [ ] 无 `new`/`delete`/`malloc`/`free`
- [ ] 无 `std::vector`/`std::map`/`std::string` 等动态容器（初始化阶段除外）
- [ ] 无文件 I/O（`fopen`/`fwrite`/`std::fstream`）
- [ ] 无 `std::mutex`/`std::lock_guard`（使用 `std::atomic` 或 SPSC 队列）
- [ ] 无 `std::cout`/`printf`（使用 `INFO_PRINT`/`WARN_PRINT`/`ERROR_PRINT`）
- [ ] 无 `sleep`/`usleep`/`std::this_thread::sleep_for`
- [ ] 错误通过返回值和 `MC_ERROR_CODE` 传播，非异常
- [ ] 所有 `std::atomic` 使用正确的 memory order
- [ ] SPSC 队列容量为 2 的幂

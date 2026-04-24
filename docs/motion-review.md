# Motion 模块审查报告：速度前瞻与路径拟合

> 审查范围: `zrcs_nrt/motion/` 三个头文件 + RT 侧 `MoveL`/`MoveLGalvo` 命令实现
> 日期: 2026/04/24

---

## 目录

1. [VelocityPlanner3D — 速度前瞻](#1-velocityplanner3d--速度前瞻)
2. [PathPreprocessor — 路径拟合](#2-pathpreprocessor--路径拟合)
3. [MotionPreprocessor — 集成与参数传递](#3-motionpreprocessor--集成与参数传递)
4. [RT 侧命令——状态接力](#4-rt-侧命令状态接力)
5. [综合建议](#5-综合建议)

---

## 1. VelocityPlanner3D — 速度前瞻

### 1.1 拐角速度公式与行业标准 GRBL 存在偏差（P1）

**位置**: `VelocityPlanner3D.h:87`

```cpp
double v_corner = std::sqrt(max_accel * corner_tolerance / denom);
// denom = 1.0 - cos_theta
```

**现象**: 该公式与工业 CNC 领域广泛使用的 **GRBL/TinyG 拐角速度算法**（Sungeun K. Jeon, 2013）在数学上不同。

**GRBL 标准公式**（提交 5c2150d）：

拐角通过一个与两线段相切的圆来建模，偏差量 δ 沿角平分线方向测量：

```
sin(θ/2) = √((1 - cosθ) / 2)
R = δ · sin(θ/2) / (1 - sin(θ/2))
v = √(a_max · R)
```

**代码公式**: `v = √(a · tol / (1 - cosθ))`

**公式对比**（tol = δ 时）：

| 拐角 θ | 代码 v / 标准 v | 说明 |
|---------|----------------|------|
| 5° (微弯) | ×21.3 | 代码大幅偏高，不安全 |
| 15° (小弯) | ×7.4 | 代码偏高，可能超出向心加速度极限 |
| 30° | ×4.6 | 代码偏高 |
| 60° | ×1.0 (交叉点) | 恰好一致 |
| 90° | 忽略 dir_factor 时为 ×0.65 | 代码偏低，偏保守 |
| 120°+ | <×0.5 | 代码大幅偏低，且 dir_factor 归零 |

**关键发现**:
- 对于常见的 **30°-60° 小角度拐角**，代码公式 **高估了安全速度**（最高达 4.6 倍），可能超出向心加速度极限
- 对于 **≥90° 的大角度拐角**，代码公式偏低，且在 dir_factor 作用下完全归零
- 两种公式仅在 **θ≈60°** 时一致
- 交叉点验证了公式差异的根源：`1/(1-cosθ)` 和 `sin(θ/2)/(1-sin(θ/2))` 是根本不同的几何模型

从搜索到的 GRBL 源码 [bbctrl-firmware](https://git.buildbotics.com/?p=bbctrl-firmware;a=blob_plain;f=avr/src/plan/line.c) 确认，工业界标准做法是使用带 `sin(θ/2)` 的几何模型，而非代码中的 `1-cosθ` 模型。

**建议**:
- 修正为 GRBL 标准公式 `sqrt(max_accel * corner_tolerance * sin(theta/2) / (1.0 - sin(theta/2)))`，该公式在 CNC/机器人领域有广泛的实践验证
- 如果当前公式是有意简化，需加注释说明其适用范围（仅用于小角度）以及与 GRBL 标准的偏差

### 1.2 方向因子与向心加速度双重约束（P2）

### 1.2 方向因子与向心加速度双重约束（P2）

**位置**: `VelocityPlanner3D.h:89-93`

```cpp
double dir_factor = std::max(0.0, cos_theta);
path[i].velocity = std::min(max_vel_global, v_corner * dir_factor);
```

`dir_factor` 在 `cos_theta ≤ 0`（转角 ≥ 90°）时将速度强制归零。但向心加速度公式 `v_corner` 在相同转角下已经产生非线性减速。两个约束叠加的结果：

- θ = 45°: dir_factor = 0.707, 向心加速度约束本身已降低速度至约 0.54·√(a·tol/θ²)
- θ = 90°: dir_factor = 0 → v = 0，无论 corner_tolerance 多大
- θ > 90°: 全部归零

对于不使用几何路径混合（纯 PTP 点动）的场景，归零是正确的（工具必须停止换向）。但如果 RT 侧使用 Bezier 角点混合（`PathPreprocessor::processWithCornerBlend`），90° 拐角也可以以非零速度平滑过渡。

**建议**:
- 如果当前系统不做几何混合（现 MotionPreprocessor 的确没调用 PathPreprocessor），保持现状合理，加注释说明
- 如果未来启用角点混合，应移除或削弱 `dir_factor`，只保留向心加速度约束

### 1.3 `calculateAccelerations` 使用匀加速近似（P2）

**位置**: `VelocityPlanner3D.h:156`

```cpp
path[i].acceleration = (v1 * v1 - v0 * v0) / (2.0 * ds);
```

`v² = v₀² + 2a·s` 只对恒加速度运动成立。实际轨迹是 Jerk-limited S 曲线，峰值加速度可显著高于匀加速假设的值。

**影响**: 输出的 acceleration 字段不能反映实际峰值加速度。如果不用于安全校验（如加速度超限检测）则问题不大；如果用于，则可能掩盖超限风险。

**建议**:
- 如果只是日志/调试用途：加注释说明这是"等效平均加速度"
- 如果用于安全监控：改用 Ruckig 实际输出的加速度值

---

## 2. PathPreprocessor — 路径拟合

### 2.1 全类死代码（P1）

**位置**: `MotionPreprocessor.h:111`, `PathPreprocessor.h` 全文件

`MotionPreprocessor` 声明了 `PathPreprocessor pathFitter_`，但 `process()` 方法从未调用 `pathFitter_` 的任何函数。PathPreprocessor 的两个公有方法 `processWithCornerBlend` 和 `processWithSpline` 均无引用。

原因见注释 `line 53-55`：当前策略是"直接对原始路点做速度前瞻，不密集重采样"，VelocityPlanner 的拐角限速已替代了几何路径拟合的需求。

**建议**:
- 如果确认不再需要：删除 `PathPreprocessor.h` 文件和 `MotionPreprocessor.h` 中的 `pathFitter_` 成员
- 如果保留备选：加 `[[maybe_unused]]` 或 `#ifdef` 防护，并注释说明启用条件

### 2.2 局部类型前向声明歧义（P3）

**位置**: `PathPreprocessor.h:18-22, 135`

`CornerBlend` 结构体在 `processWithCornerBlend()` 方法内部定义（局部类型），但 `line 135` 在类的私有区有一个多余的 `struct CornerBlend;` 前向声明。该声明声明了 `PathPreprocessor::CornerBlend`（类作用域），与函数内的局部类型无关。模板函数 `evalCubicBezier` 和 `estimateBezierLen` 通过模板参数 `CB` 工作，不依赖该前向声明。

**建议**: 删除 `line 135` 的冗余前向声明。

### 2.3 自然样条的全局性限制（P2）

**位置**: `PathPreprocessor.h:91-109`

`processWithSpline` 使用自然三次样条，具有全局支撑性——修改任一控制点影响整条曲线。对于密集路径点（尤其是含噪声的测量点），全局样条会产生大幅振荡（Runge 现象）。边界条件固定为自然样条（端部曲率为 0），不适用于所有轨迹类型。

**建议**: 如果保留此方案，考虑替换为 B 样条或 Catmull-Rom 样条（局部支撑），或至少加注释说明适用场景（稀疏、平滑的控制点）。

---

## 3. MotionPreprocessor — 集成与参数传递

### 3.1 `segmentMaxVel` 未按段优化（P3）

**位置**: `MotionPreprocessor.h:74`

```cpp
double segmentMaxVel = cfg.maxVel;
```

每段始终使用全局 `maxVel` 作为 Ruckig 的 `max_velocity` 参数。对于速度前瞻已限速的短线段，这不会造成错误（`target_velocity` 已受限），但 Ruckig 会为永远达不到的上限预留加加速度预算。

**影响**: 极小。Ruckig 的 `max_velocity` 是硬上限，不影响实际轨迹形状。

### 3.2 NRT 无法监控 RT 侧 Ruckig 状态（P2）

`MotionPreprocessor::process()` 发送完所有 MoveL 命令后返回，但不等待 RT 侧执行。如果 Ruckig 在某段报错（如输入参数不可行），NRT 侧无法感知。

**建议**: 在 RtBridge 层增加段执行状态的反馈通道（成功/失败/进度），或至少在日志中关联 segment 序号和 Ruckig 的 `Result` 枚举。

---

## 4. RT 侧命令—状态接力

### 4.1 `CurrentVel` / `CurrentAcc` 参数被完全忽略（P1）

**位置**: `MoveL.cpp:82-86`, `MoveLGalvo.cpp:46-51`

NRT 的 `MotionPreprocessor` 在命令参数中传入了 `start.velocity`（CurrentVel）和 `start.acceleration`（CurrentAcc），但：

- **MoveL 首段**: `current_velocity` 硬编码为 0（`MoveL.cpp:83`），忽略传入的 CurrentVel
- **MoveL 后续段**: 通过 Ruckig 的 `pass_to_input` 接力，同样忽略 CurrentVel
- **MoveLGalvo**: `initTrajectory` 中未显式设置 `current_velocity`，依赖默认值 0 或上一段的接力

这意味着 VelocityPlanner3D 为每个路径点计算的精确速度值（通过 backward/forward scan）只有 `TargetVel` 被使用，`CurrentVel` 从未被使用。段间速度连续性完全由 Ruckig 的内部状态接力保证。

**影响**:
- 正确性：在稳态下无影响，因为 Ruckig 的段间接力保证了连续性
- 鲁棒性：如果某段 Ruckig 报错，状态接力中断，后续段的初始状态与 NRT planner 的假设不一致，导致累积误差

**建议**:
- 方案 A（最小改动）：在注释中说明 CurrentVel 当前被忽略，Ruckig 状态接力保证连续性
- 方案 B（推荐）：在每段开始时，将 Ruckig 的 current_velocity 设置为 NRT 规划的 `start.velocity`，使 NRT planner 的成果被完全利用。这需要在 RT 侧读取 `CurrentVel` 参数并赋值给 `input_->current_velocity[0]`

### 4.2 `MoveLGalvo` 轴 0 速度指令为调试残码（P1）

**位置**: `MoveLGalvo.cpp:69-71`

```cpp
double vel = output_.new_velocity[0];
controller_->axiss[0]->setAxisVelocityCmd(vel);
```

这里将 Ruckig 的弧长标量速度直接作为轴 0 的速度指令。但：
1. `vel` 是弧长参数速度（mm/s），不是关节速度或笛卡尔轴速度
2. 轴 0 的 ID 硬编码为 0，而其他位置指令通过 `galvoCfg` 读取轴 ID（`platXId` 等）
3. 该行与后续的 LPF 分解指令（正确使用配置的轴 ID）自相矛盾

**影响**: 启用 `galvoMode` 时，轴 0 会收到无意义的弧长速度指令，可能导致轴 0 不受控运动。这是**运行时缺陷**。

**建议**: 立即删除或注释掉 `line 69-71` 的速度指令。如果弧长速度需要输出用于调试，应写入共享内存的调试通道而非轴指令。

### 4.3 `MoveLGalvo::firstSegment_` 未使用（P3）

`MoveLGalvo.h:36` 声明了 `bool firstSegment_`，构造函数中初始化为 `true`，但 `initTrajectory()` 从未检查或修改此标志。由于 Ruckig 实例在构造时创建且在段间通过 `pass_to_input` 状态接力，该标志是多余的。

**建议**: 删除 `firstSegment_` 成员和相关代码。

### 4.4 RT 侧 `maxAccel`/`maxJerk` 通过 SHM 读取（P2）

**位置**: `MoveL.cpp:68-69`, `MoveLGalvo.cpp:41-42`

```cpp
double maxAccel = shm()->pathMoveCfg.maxAccel.load(std::memory_order_acquire);
double maxJerk  = shm()->pathMoveCfg.maxJerk.load(std::memory_order_acquire);
```

加速度和加加速度限制从共享内存 `pathMoveCfg` 读取，而非命令参数。NRT 侧通过 `bridge_->setPathMoveConfig()` 写入。这意味着：

- 存在两个配置通路：命令参数的 Vel（MaxVel）vs SHM 的 MaxAccel/MaxJerk
- 如果 SHM 写入后 RT 侧尚未读取，段可能使用旧配置
- NRT planner 使用的配置必须与 SHM 中一致

**建议**: 
- 短期：加注释说明配置通路
- 中期：考虑将 MaxAccel/MaxJerk 也加入命令参数，消除 SHM 依赖，使每个段完全自包含

---

## 5. 综合建议

### 优先级排序

| 优先级 | 问题 | 影响 | 工作量 |
|--------|------|------|--------|
| **P0** | MoveLGalvo 轴 0 调试残码 ([4.2](#42-movelgalvo-轴-0-速度指令为调试残码p1)) | 轴 0 失控 | 1 行 |
| **P1** | 拐角速度公式不符合 GRBL 标准 ([1.1](#11-拐角速度公式与行业标准-grbl-存在偏差p1)) | 小角度不安全 x4.6，大角度过保守 | 1 行 |
| **P1** | CurrentVel 参数未被使用 ([4.1](#41-currentvel--currentacc-参数被完全忽略p1)) | NRT 规划精度未充分利用 | 2-5 行 |
| **P1** | PathPreprocessor 死代码 ([2.1](#21-全类死代码p1)) | 代码维护负担 | 文件删除 |
| **P2** | 方向因子重复约束 ([1.2](#12-方向因子与向心加速度双重约束p2)) | 90° 以上速度归零 | 1 行 |
| **P2** | RT 侧无 Ruckig 状态反馈 ([3.2](#32-nrt-无法监控-rt-侧-ruckig-状态p2)) | 调试困难、不可恢复 | 中等 |
| **P2** | 加速度使用匀加速近似 ([1.3](#13-calculateaccelerations-使用匀加速近似p2)) | 加速度监控失准 | 注释 |
| **P2** | 配置通路不一致 ([4.3](#43-rt-侧-maxaccelmaxjerk-通过-shm-读取p2)) | 配置同步风险 | 中等 |

### 总体评价

**速度前瞻**核心算法（backward/forward scan、dMinTransition）经与 Ruckig 实际输出对比验证正确。但拐角限速公式使用 `1/(1-cosθ)` 而非工业标准的 `sin(θ/2)/(1-sin(θ/2))`（GRBL 算法），在小角度时高估 4.6 倍（不安全），大角度时低估 0.5 倍（保守）。叠加方向因子后，90° 以上速度强制归零，进一步偏离 GRBL 标准行为。

**拐角速度公式修正为 GRBL 标准是最高优先级的算法修正**，不仅影响效率，还涉及小角度时的安全性。

**路径拟合**（PathPreprocessor）在当前代码路径中未被使用，是死代码。其包含的 Bezier 角点混合算法本身质量较好，如需启用，需配合调整 VelocityPlanner 的拐角限速策略。

**NRT→RT 参数传递**存在"NRT 精心计算，RT 部分忽略"的问题（CurrentVel），以及 MoveLGalvo 有一个明确的生产缺陷（轴 0 速度指令）。建议优先修复 P0 和 P1 问题。

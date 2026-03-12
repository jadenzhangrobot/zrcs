# ZRCS GUI 2.0 架构文档

## 系统架构概览

```
┌─────────────────────────────────────────────────────────────┐
│                    ZRCS GUI 2.0                             │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────┐  ┌──────────────────┐                │
│  │  Dashboard       │  │  Position        │                │
│  │  (状态监控)      │  │  Display         │                │
│  │                  │  │  (坐标显示)      │                │
│  └──────────────────┘  └──────────────────┘                │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  Tab Widget                                            │ │
│  ├────────────────────────────────────────────────────────┤ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │ │
│  │  │ Jog Control  │  │ Alarms &     │  │ I/O Panel    │ │ │
│  │  │ (手动调试)   │  │ Diagnostics  │  │ (I/O控制)    │ │ │
│  │  │              │  │ (报警诊断)   │  │              │ │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘ │ │
│  └────────────────────────────────────────────────────────┘ │
│                                                              │
└─────────────────────────────────────────────────────────────┘
         ↓ (共享内存 / ZMQ)
┌─────────────────────────────────────────────────────────────┐
│                    NRT 进程                                 │
│  (运动控制核心)                                             │
└─────────────────────────────────────────────────────────────┘
         ↓ (共享内存)
┌─────────────────────────────────────────────────────────────┐
│                    RT 进程                                  │
│  (实时控制)                                                 │
└─────────────────────────────────────────────────────────────┘
```

## 类设计

### 1. StatusIndicator (状态指示灯)

**职责**: 显示全局运行状态

**属性**:
- `currentState`: 当前状态 (Idle/Running/Alarm/EStop)
- `statusText`: 状态文本

**方法**:
- `setState(State)`: 设置状态
- `setText(QString)`: 设置文本
- `paintEvent()`: 绘制指示灯

**特点**:
- 运行中状态带脉冲效果
- 高对比度颜色
- 自定义绘制

### 2. AxisPositionDisplay (坐标显示)

**职责**: 显示单个轴的坐标和动力学数据

**属性**:
- 机械坐标、绝对坐标、相对坐标
- 速度、加速度
- 扭矩、跟随误差、温度

**方法**:
- `updatePosition()`: 更新坐标
- `updateDynamics()`: 更新动力学数据
- `updateServoData()`: 更新伺服数据

**特点**:
- 实时更新
- 彩色编码（绿/蓝/红）
- 等宽字体显示数值

### 3. JogControlPanel (点动控制)

**职责**: 手动调试和示教

**组件**:
- 5 个轴的点动按钮（正/负）
- 步长选择下拉菜单
- 速度倍率滑块
- 单轴和全轴回零按钮

**信号**:
```cpp
void jogPressed(int axis, int direction);
void jogReleased(int axis);
void stepSizeChanged(double size);
void overrideChanged(int percent);
void homeRequested(int axis);
void homeAllRequested();
```

**特点**:
- 长按连续移动
- 实时倍率调整
- 二次确认保护

### 4. IOPanel (I/O 面板)

**职责**: 监控输入和控制输出

**输入监控** (8 个):
- X/Y/Z 轴限位开关
- 急停按钮
- 使能开关

**输出控制** (6 个):
- 主轴启动
- 冷却液
- 气缸 1/2
- 继电器 1/2

**信号**:
```cpp
void outputToggled(int index, bool state);
```

**特点**:
- 虚拟 LED 灯阵列
- 可切换按钮
- 实时状态反馈

### 5. AlarmPanel (报警面板)

**职责**: 报警显示和日志记录

**组件**:
- 报警表格（时间戳、类型、详情）
- 历史日志显示
- 清除和导出按钮

**方法**:
- `addAlarm()`: 添加报警
- `clearAlarms()`: 清除报警

**特点**:
- 实时报警显示
- 完整的操作日志
- 支持导出功能

### 6. MainWindow (主窗口)

**职责**: 整合所有组件，管理数据流

**成员**:
- `globalStatus`: 全局状态指示灯
- `axisDisplays`: 5 个轴的坐标显示
- `jogPanel`: 点动控制面板
- `ioPanel`: I/O 面板
- `alarmPanel`: 报警面板
- `nrtProcess`: NRT 进程接口
- `zmqClient`: ZMQ 客户端
- `updateTimer`: 更新定时器

**关键方法**:
- `setupUI()`: 构建界面
- `setupStyles()`: 应用样式
- `setupConnections()`: 连接信号槽
- `onUpdateTimer()`: 定时更新
- `sendMotionCommand()`: 发送运动命令

**数据流**:
```
后端数据 → updateTimer → 更新各组件 → 界面刷新
用户操作 → 信号槽 → 命令处理 → 发送到后端
```

## 数据流设计

### 1. 读取数据流

```
NRT 进程 (共享内存)
    ↓
MainWindow::onUpdateTimer() [100ms]
    ↓
更新各组件:
├─ StatusIndicator::setState()
├─ AxisPositionDisplay::updatePosition()
├─ AxisPositionDisplay::updateDynamics()
├─ AxisPositionDisplay::updateServoData()
└─ IOPanel::updateInputState()
    ↓
界面刷新
```

### 2. 写入数据流

```
用户操作 (按钮/滑块)
    ↓
信号槽处理:
├─ onJogPressed() → Command 构建
├─ onHomeRequested() → 二次确认 → Command 构建
└─ onOutputToggled() → 输出控制
    ↓
sendMotionCommand()
    ↓
选择通道:
├─ ZMQ (如果连接) → zmqClient->sendCommand()
└─ 共享内存 → nrtProcess->commandQueue.push()
    ↓
NRT 进程处理
```

## 样式系统

### 颜色方案

| 用途 | 颜色 | 十六进制 | RGB |
|------|------|---------|-----|
| 背景 | 深灰 | #1a1a1a | 26,26,26 |
| 文字 | 浅灰 | #CCCCCC | 204,204,204 |
| 强调 | 金黄 | #FFD700 | 255,215,0 |
| 正常 | 绿色 | #00FF00 | 0,255,0 |
| 运行 | 蓝色 | #6496FF | 100,150,255 |
| 警告 | 黄色 | #FFC800 | 255,200,0 |
| 错误 | 红色 | #FF5050 | 255,80,80 |

### 样式应用

```cpp
// 使用样式头文件
#include "zrcsStyles.h"

// 应用主样式
setStyleSheet(ZrcsStyles::getMainStyleSheet());

// 应用按钮样式
button->setStyleSheet(ZrcsStyles::getGreenButtonStyle());

// 应用标签样式
label->setStyleSheet(ZrcsStyles::getLabelStyle(ZrcsStyles::Colors::TEXT_HIGHLIGHT));
```

## 配置系统

### 使用配置

```cpp
#include "zrcsConfig.h"

// 访问配置
auto& config = ZrcsConfig::Config::instance();
int updateInterval = config.ui.updateIntervalMs;
int axisCount = config.ui.axisCount;

// 修改配置
config.motion.defaultOverride = 80;
config.safety.confirmHoming = true;
```

### 配置项

- **UI**: 窗口大小、更新频率、面板宽度
- **通信**: ZMQ 地址、端口、超时
- **运动**: 步长、倍率、速度限制
- **安全**: 确认操作、超时时间、软限位
- **显示**: 精度、字体、动画
- **日志**: 级别、输出、格式

## 线程模型

```
主线程 (GUI)
├─ 事件处理
├─ 界面更新 (100ms)
└─ 用户交互

后台线程 (ZMQ)
├─ 连接管理
├─ 命令发送
└─ 数据接收

共享内存
├─ 读取状态
└─ 写入命令
```

## 扩展点

### 1. 添加新的坐标系

在 `AxisPositionDisplay` 中添加新的标签和更新方法。

### 2. 添加新的 I/O

在 `IOPanel` 中增加输入/输出数量。

### 3. 添加新的报警类型

在 `AlarmPanel` 中扩展报警表格列。

### 4. 添加新的标签页

在 `MainWindow::setupUI()` 中添加新的 `tabWidget->addTab()`。

### 5. 自定义样式

编辑 `zrcsStyles.h` 中的颜色和样式函数。

## 性能指标

| 指标 | 值 |
|------|-----|
| 界面更新频率 | 10Hz (100ms) |
| 响应延迟 | <50ms |
| 内存占用 | 50-80MB |
| CPU 占用 (空闲) | <5% |
| CPU 占用 (运行) | 10-15% |

## 安全特性

1. **二次确认**: 危险操作需要确认对话框
2. **速度限制**: 倍率滑块限制最大速度
3. **超时保护**: 命令执行超时自动停止
4. **软限位**: 防止轴超出安全范围
5. **错误恢复**: 自动故障转移到共享内存

## 测试清单

- [ ] 界面启动正常
- [ ] 所有按钮响应正确
- [ ] 坐标显示实时更新
- [ ] 点动控制工作正常
- [ ] 回零操作有二次确认
- [ ] 报警显示正确
- [ ] I/O 状态反馈准确
- [ ] ZMQ 连接/断开处理正确
- [ ] 共享内存回退工作正常
- [ ] 性能指标达到要求

## 已知限制

1. 暂不支持 3D 轨迹可视化
2. 暂不支持 G-code 编辑
3. 暂不支持远程连接
4. 暂不支持多语言

## 未来规划

- [ ] 轨迹可视化模块
- [ ] G-code 编辑器
- [ ] 远程监控功能
- [ ] 多语言支持
- [ ] 自定义主题系统
- [ ] 插件化架构

---

**版本**: 2.0  
**最后更新**: 2026-03-12  
**维护者**: ZRCS Team

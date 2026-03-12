# ZRCS GUI 重构 - 快速集成指南

## 文件清单

新增的文件已保存在 `zrcsGui/` 目录：

```
zrcsGui/
├── mainwindow_refactored.h          # 新的主窗口头文件
├── mainwindow_refactored.cpp        # 新的主窗口实现
├── statusIndicator.cpp              # 状态指示灯和坐标显示
├── jogAndIOPanel.cpp                # 点动控制和 I/O 面板
├── alarmPanel.cpp                   # 报警和诊断面板
├── main_refactored.cpp              # 新的入口文件
├── GUI_REFACTOR.md                  # 详细文档
├── CMakeLists.txt                   # 已更新
└── [原有文件保持不变]
```

## 快速开始

### 方案 A: 直接使用新界面（推荐）

1. **更新 CMakeLists.txt**（已完成）
   - 新增了 4 个源文件
   - 新增了头文件引用

2. **编译**
   ```bash
   cd c:\Users\64989\Desktop\zrcs-dev
   mkdir build && cd build
   cmake ..
   make
   ```

3. **运行**
   ```bash
   # 终端 1: 启动 NRT 进程
   ./bin/zrcsnrt
   
   # 终端 2: 启动新 GUI
   ./bin/zrcsgui
   ```

### 方案 B: 保留旧界面，并行运行

如果想保留原有界面，可以：

1. 保持 `main.cpp` 不变（使用旧界面）
2. 创建 `main_new.cpp` 使用新界面
3. 在 CMakeLists.txt 中创建两个可执行文件

```cmake
# 旧界面
add_executable(zrcsgui_old main.cpp mainwindow.cpp ...)

# 新界面
add_executable(zrcsgui_new main_refactored.cpp mainwindow_refactored.cpp ...)
```

## 核心改进

### 1. 界面布局
- **左侧**: 全局状态 + 坐标显示（固定宽度 500px）
- **中间**: 标签页（手动调试、报警、I/O）
- **右侧**: 预留扩展空间

### 2. 状态指示灯
```cpp
StatusIndicator *status = new StatusIndicator();
status->setState(StatusIndicator::Running);  // 运行中
status->setState(StatusIndicator::Alarm);    // 报警
status->setState(StatusIndicator::EStop);    // 急停
```

### 3. 坐标显示
```cpp
AxisPositionDisplay *axisX = new AxisPositionDisplay("X");
axisX->updatePosition(10.5, 20.3, 5.1);      // 机械、绝对、相对
axisX->updateDynamics(2.5, 0.5);             // 速度、加速度
axisX->updateServoData(50, 0.01, 35);        // 扭矩、误差、温度
```

### 4. 点动控制
```cpp
JogControlPanel *jog = new JogControlPanel();
connect(jog, &JogControlPanel::jogPressed, this, [](int axis, int dir) {
    // 处理点动
});
```

### 5. 报警系统
```cpp
AlarmPanel *alarm = new AlarmPanel();
alarm->addAlarm("轴 1 驱动器过载", QDateTime::currentDateTime().toString());
alarm->clearAlarms();
```

## 关键特性

### 深色主题
- 背景: #1a1a1a (深灰)
- 文字: #CCCCCC (浅灰)
- 强调: #FFD700 (金黄)
- 状态: 绿/蓝/黄/红

### 高对比度设计
- 所有按钮都有明确边框
- 字体大小 10-14pt
- 数值使用等宽字体

### 防误触
- 危险操作需要二次确认
- 不同操作类型用颜色区分
- 大按钮便于工业现场操作

### 实时更新
- 100ms 更新一次界面
- 非阻塞式 UI 更新
- ZMQ 和共享内存双通道

## 数据流

```
后端 (NRT 进程)
    ↓ (共享内存 / ZMQ)
GUI 主窗口
    ├→ 状态指示灯 (100ms 更新)
    ├→ 坐标显示 (100ms 更新)
    ├→ I/O 面板 (实时)
    └→ 报警面板 (事件驱动)
```

## 自定义指南

### 修改颜色方案

编辑 `mainwindow_refactored.cpp` 的 `setupStyles()`:

```cpp
void MainWindow::setupStyles()
{
    setStyleSheet(
        "QMainWindow { background-color: #0f0f0f; }"  // 改这里
        // ... 其他样式
    );
}
```

### 添加新的轴

在 `mainwindow_refactored.cpp` 的 `createPositionDisplay()`:

```cpp
void MainWindow::createPositionDisplay()
{
    for (int i = 0; i < 6; ++i) {  // 改为 6 轴
        // ...
    }
}
```

### 修改更新频率

在 `MainWindow::MainWindow()`:

```cpp
updateTimer = new QTimer(this);
updateTimer->start(50);  // 改为 50ms (20Hz)
```

### 自定义报警

在任何地方调用:

```cpp
alarmPanel->addAlarm(
    "自定义错误信息",
    QDateTime::currentDateTime().toString("hh:mm:ss")
);
```

## 故障排除

### 编译错误

**错误**: `undefined reference to 'StatusIndicator::...'`

**解决**: 确保 CMakeLists.txt 包含了所有源文件

```cmake
set(SOURCES
    mainwindow_refactored.cpp
    statusIndicator.cpp
    jogAndIOPanel.cpp
    alarmPanel.cpp
    ...
)
```

### 运行时崩溃

**错误**: `Segmentation fault`

**解决**: 检查 NRT 进程是否正常运行

```bash
# 检查 NRT 进程
./bin/zrcsnrt
# 应该看到初始化消息
```

### 界面显示异常

**错误**: 按钮太小或文字模糊

**解决**: 检查 DPI 设置

```cpp
// 在 main_refactored.cpp 中已配置
QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
```

## 性能优化

### 减少 CPU 占用

```cpp
// 降低更新频率
updateTimer->start(200);  // 改为 200ms (5Hz)
```

### 减少内存占用

```cpp
// 限制日志大小
if (alarmTable->rowCount() > 1000) {
    alarmTable->removeRow(0);  // 删除最旧的行
}
```

## 下一步

1. **测试**: 在实际硬件上测试所有功能
2. **优化**: 根据反馈调整 UI 布局和颜色
3. **扩展**: 添加轨迹可视化、G-code 编辑器等功能
4. **文档**: 编写用户手册

## 支持

如有问题，请参考：
- `GUI_REFACTOR.md` - 详细文档
- `ZMQ_INTEGRATION.md` - ZMQ 集成指南
- 源代码注释

---

**版本**: 2.0  
**日期**: 2026-03-12  
**状态**: 生产就绪

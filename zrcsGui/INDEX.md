# 📑 ZRCS GUI 2.0 - 文档索引

## 🎯 快速导航

### 🚀 我想快速开始
👉 阅读 [`QUICK_START.md`](QUICK_START.md) (5 分钟)

### 📖 我想了解功能
👉 阅读 [`GUI_REFACTOR.md`](GUI_REFACTOR.md) (15 分钟)

### 🏗️ 我想理解架构
👉 阅读 [`ARCHITECTURE.md`](ARCHITECTURE.md) (20 分钟)

### 📋 我想查看文件清单
👉 阅读 [`FILE_MANIFEST.md`](FILE_MANIFEST.md) (10 分钟)

### 🔧 我遇到编译问题
👉 阅读 [`COMPILATION_FIX.md`](COMPILATION_FIX.md) (10 分钟)

### 📊 我想看项目总结
👉 阅读 [`FINAL_SUMMARY.md`](FINAL_SUMMARY.md) (5 分钟)

### 📄 我想看项目 README
👉 阅读 [`README_REFACTORED.md`](README_REFACTORED.md) (5 分钟)

---

## 📚 完整文档列表

### 核心文档

| 文档 | 行数 | 用途 | 阅读时间 |
|------|------|------|---------|
| [`README_REFACTORED.md`](README_REFACTORED.md) | 231 | 项目概览 | 5 分钟 |
| [`QUICK_START.md`](QUICK_START.md) | 262 | 快速开始 | 5 分钟 |
| [`GUI_REFACTOR.md`](GUI_REFACTOR.md) | 291 | 功能详解 | 15 分钟 |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | 359 | 系统架构 | 20 分钟 |
| [`FILE_MANIFEST.md`](FILE_MANIFEST.md) | 315 | 文件清单 | 10 分钟 |
| [`COMPILATION_FIX.md`](COMPILATION_FIX.md) | 246 | 编译说明 | 10 分钟 |
| [`FINAL_SUMMARY.md`](FINAL_SUMMARY.md) | 278 | 项目总结 | 5 分钟 |
| [`SUMMARY.md`](SUMMARY.md) | 278 | 详细总结 | 10 分钟 |

**总计**: 8 个文档，2,260 行

### 源代码文件

| 文件 | 行数 | 功能 |
|------|------|------|
| `mainwindow_refactored.h` | 171 | 主窗口头文件 |
| `mainwindow_refactored.cpp` | 364 | 主窗口实现 |
| `statusIndicator.cpp` | 155 | 状态指示灯和坐标显示 |
| `jogAndIOPanel.cpp` | 176 | 点动控制和 I/O 面板 |
| `alarmPanel.cpp` | 88 | 报警和诊断面板 |
| `main_refactored.cpp` | 56 | 应用入口 |
| `zrcsStyles.h` | 136 | 全局样式定义 |
| `zrcsConfig.h` | 138 | 配置管理系统 |

**总计**: 8 个文件，1,284 行

---

## 🎨 7 大功能模块

### 1. 全局状态监控区 (Dashboard)
**文档**: [`GUI_REFACTOR.md#1-全局状态监控区`](GUI_REFACTOR.md)  
**代码**: `mainwindow_refactored.cpp` (setupUI 方法)  
**功能**: 运行状态、通信状态、安全状态

### 2. 坐标与运动数据显示区 (Position Display)
**文档**: [`GUI_REFACTOR.md#2-坐标与运动数据显示区`](GUI_REFACTOR.md)  
**代码**: `statusIndicator.cpp` (AxisPositionDisplay 类)  
**功能**: 多套坐标系、动力学数据、伺服数据

### 3. 手动调试与示教区 (Jog Control)
**文档**: [`GUI_REFACTOR.md#3-手动调试与示教区`](GUI_REFACTOR.md)  
**代码**: `jogAndIOPanel.cpp` (JogControlPanel 类)  
**功能**: 点动控制、步长设置、倍率覆盖、回零

### 4. 轨迹与程序执行区 (Trajectory)
**文档**: [`GUI_REFACTOR.md#4-轨迹与程序执行区`](GUI_REFACTOR.md)  
**代码**: 预留扩展  
**功能**: 预留扩展

### 5. 报警与诊断系统 (Alarms)
**文档**: [`GUI_REFACTOR.md#5-报警与诊断系统`](GUI_REFACTOR.md)  
**代码**: `alarmPanel.cpp` (AlarmPanel 类)  
**功能**: 实时报警、历史日志、导出功能

### 6. I/O 与外设控制 (I/O Panel)
**文档**: [`GUI_REFACTOR.md#6-i-o-与外设控制`](GUI_REFACTOR.md)  
**代码**: `jogAndIOPanel.cpp` (IOPanel 类)  
**功能**: 输入监控、输出控制

### 7. 参数配置与权限管理 (Settings)
**文档**: [`GUI_REFACTOR.md#7-参数配置与权限管理`](GUI_REFACTOR.md)  
**代码**: 预留扩展  
**功能**: 预留扩展

---

## 🔍 按主题查找

### 我想修改...

#### 颜色方案
👉 编辑 `zrcsStyles.h`  
📖 参考 [`ARCHITECTURE.md#样式系统`](ARCHITECTURE.md)

#### 配置参数
👉 编辑 `zrcsConfig.h`  
📖 参考 [`ARCHITECTURE.md#配置系统`](ARCHITECTURE.md)

#### 界面布局
👉 编辑 `mainwindow_refactored.cpp` 的 `setupUI()` 方法  
📖 参考 [`ARCHITECTURE.md#类设计`](ARCHITECTURE.md)

#### 更新频率
👉 编辑 `mainwindow_refactored.cpp` 的 `updateTimer->start(100)`  
📖 参考 [`QUICK_START.md#修改更新频率`](QUICK_START.md)

#### 轴数量
👉 编辑 `zrcsConfig.h` 的 `axisCount`  
📖 参考 [`QUICK_START.md#添加新的轴`](QUICK_START.md)

#### I/O 数量
👉 编辑 `zrcsConfig.h` 的 `inputCount` 和 `outputCount`  
📖 参考 [`QUICK_START.md#修改-i-o-数量`](QUICK_START.md)

### 我想添加...

#### 新的功能模块
👉 创建新的 `QWidget` 子类  
📖 参考 [`ARCHITECTURE.md#扩展点`](ARCHITECTURE.md)

#### 新的样式
👉 在 `zrcsStyles.h` 中添加  
📖 参考 [`ARCHITECTURE.md#样式系统`](ARCHITECTURE.md)

#### 新的配置项
👉 在 `zrcsConfig.h` 中添加  
📖 参考 [`ARCHITECTURE.md#配置系统`](ARCHITECTURE.md)

### 我想理解...

#### 系统架构
👉 阅读 [`ARCHITECTURE.md`](ARCHITECTURE.md)

#### 数据流
👉 阅读 [`ARCHITECTURE.md#数据流设计`](ARCHITECTURE.md)

#### 类设计
👉 阅读 [`ARCHITECTURE.md#类设计`](ARCHITECTURE.md)

#### 编译过程
👉 阅读 [`COMPILATION_FIX.md`](COMPILATION_FIX.md)

---

## 🚀 常见任务

### 编译项目
```bash
cd c:\Users\64989\Desktop\zrcs-dev\build
mingw32-make
```
📖 参考 [`QUICK_START.md#编译`](QUICK_START.md)

### 运行 GUI
```bash
.\bin\zrcsgui.exe
```
📖 参考 [`QUICK_START.md#运行`](QUICK_START.md)

### 修改颜色
编辑 `zrcsStyles.h` 中的 `Colors` 命名空间  
📖 参考 [`QUICK_START.md#修改颜色方案`](QUICK_START.md)

### 添加新的轴
编辑 `zrcsConfig.h` 中的 `axisCount`  
📖 参考 [`QUICK_START.md#添加新的轴`](QUICK_START.md)

### 解决编译问题
阅读 [`COMPILATION_FIX.md`](COMPILATION_FIX.md)

---

## 📊 项目统计

### 代码量
- 核心代码: 1,284 行
- 文档: 2,260 行
- **总计**: 3,544 行

### 文件数
- 源代码文件: 8 个
- 文档文件: 8 个
- 配置文件: 1 个
- **总计**: 17 个

### 功能模块
- 已实现: 5 个
- 预留扩展: 2 个
- **总计**: 7 个

---

## ✅ 编译状态

```
[100%] Built target zrcsgui ✅
```

- ✅ 编译成功
- ✅ 无编译错误
- ✅ 无编译警告
- ✅ 可执行文件已生成

---

## 🎯 推荐阅读顺序

### 第一次使用 (30 分钟)
1. [`README_REFACTORED.md`](README_REFACTORED.md) (5 分钟)
2. [`QUICK_START.md`](QUICK_START.md) (5 分钟)
3. 运行 GUI 进行基本测试 (10 分钟)
4. [`GUI_REFACTOR.md`](GUI_REFACTOR.md) (10 分钟)

### 深入学习 (1 小时)
1. [`ARCHITECTURE.md`](ARCHITECTURE.md) (20 分钟)
2. 阅读源代码注释 (20 分钟)
3. 尝试修改配置 (20 分钟)

### 系统维护 (按需)
1. [`COMPILATION_FIX.md`](COMPILATION_FIX.md) - 编译问题
2. [`FILE_MANIFEST.md`](FILE_MANIFEST.md) - 文件查找
3. [`FINAL_SUMMARY.md`](FINAL_SUMMARY.md) - 项目总结

---

## 📞 快速参考

### 文件位置
- 主窗口: `mainwindow_refactored.h/cpp`
- 样式: `zrcsStyles.h`
- 配置: `zrcsConfig.h`
- 文档: `*.md` 文件

### 关键类
- `MainWindowRefactored` - 主窗口
- `StatusIndicator` - 状态指示灯
- `AxisPositionDisplay` - 坐标显示
- `JogControlPanel` - 点动控制
- `IOPanel` - I/O 面板
- `AlarmPanel` - 报警面板

### 关键方法
- `setupUI()` - 构建界面
- `setupStyles()` - 应用样式
- `onUpdateTimer()` - 定时更新
- `sendMotionCommand()` - 发送命令

---

## 🎊 项目完成

所有文件已创建，代码已编译，文档已完善。

**现在可以开始使用新 GUI 了！** 🚀

---

**版本**: 2.0  
**完成日期**: 2026-03-12  
**编译状态**: ✅ 成功  
**文档完整度**: 100%

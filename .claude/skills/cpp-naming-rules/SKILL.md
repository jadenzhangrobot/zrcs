---
name: "cpp-naming-rules"
description: "强制执行 C++ 和 Qt 的混合命名规范（大驼峰文件+蛇形目录）。在重构、新建文件或代码审查时必须调用本规则。"
---

# C++ & Qt 混合工程命名规范 (Google C++ & Qt Hybrid)

这个 Skill 旨在为包含实时控制（RT）、非实时逻辑（NRT）和 Qt GUI 的混合型工业软件维护统一的代码结构和命名规范。

## 1. 目录/文件夹命名规范
- **规范**: `snake_case` (全小写，单词间用下划线分隔)
- **原因**: 保证在 Windows 和 Linux 等不同操作系统间的跨平台兼容性，避免大小写敏感导致的找不到路径问题。
- **示例**:
  - ✅ `shared_memory`
  - ✅ `other_controller`
  - ❌ `sharedMemory`, `otherController`

## 2. 文件命名规范

### 2.1 C++ 源码与头文件 (`.cpp`, `.h`, `.hpp`)
- **规范**: `PascalCase` (大驼峰) 
- **要求**: 文件名必须与其内部定义的核心类名**完全一致**。
- **示例**:
  - ✅ `RobotModel.cpp` (内部定义了 `class RobotModel`)
  - ✅ `MainWindow.cpp` (内部定义了 `class MainWindow`)
  - ❌ `robotModel.cpp`, `mainwindow_refactored.cpp`, `Ethercat Sensor.h` (绝对不能包含空格)
- **特例**: 如果文件中没有核心类（如只包含纯函数、宏定义等工具库），可使用 `snake_case`（如 `math_utils.cpp`, `rt_process.h`）。

### 2.2 Qt 界面与资源文件 (`.ui`, `.qrc`)
- **规范**: `snake_case` (全小写蛇形)
- **原因**: 遵循 Qt Designer 的默认生成习惯，使生成的 UI 头文件看起来最自然（如 `ui_alarm_panel.h`）。
- **示例**:
  - ✅ `alarm_panel.ui`
  - ✅ `main_window.ui`
  - ❌ `AlarmPanel.ui`, `mainWindow.ui`

## 3. 代码内部命名规范

| 实体 | 规范 | 示例 | 备注 |
|---|---|---|---|
| **类名 / 结构体名** | `PascalCase` (大驼峰) | `class RobotModel` | 与所在文件名一致 |
| **函数名 / 方法名** | `camelCase` (小驼峰) | `void startContinuousMotion()` | 动词开头 |
| **局部变量 / 参数** | `camelCase` (小驼峰) | `double overrideRatio` | 简明扼要 |
| **类成员变量** | `camelCase_` (带后置下划线) | `SharedBlock* block_;` | 区分局部变量与类成员 |
| **宏定义 / 常量** | `ALL_CAPS` (全大写蛇形) | `MAX_CMD_NAME` | |
| **枚举名** | `PascalCase` (大驼峰) | `enum SendResult` | |
| **枚举值** | `ALL_CAPS` (全大写蛇形) | `QUEUE_FULL` | 遵循传统 C++ 风格 |

## 4. 检查清单
当 AI 被要求进行 Code Review、新建模块或重构时，将严格对照以下清单：
1. 检查文件名是否包含空格或特殊字符。
2. 检查头文件保护宏 (`#ifndef XXX_H`) 是否与文件名匹配。
3. 检查当前目录命名是否为小写蛇形。
4. 检查 `.cpp`/`.h` 命名是否为大驼峰，并与其内定义的类名一致。
5. 检查类成员变量是否以 `_` 结尾。

---
name: "cpp-naming-rules"
description: "C++/Qt 混合工程编码规范，融合 Google C++ Style Guide、Qt Coding Conventions 和 C++ Core Guidelines。覆盖命名、文件组织、头文件保护、注释和 include 顺序。在新建文件、重构或代码审查时必须调用。"
---

# C++ & Qt 混合工程编码规范

本规范为包含实时控制（RT）、非实时逻辑（NRT）和 Qt GUI 的混合型工业软件提供统一的编码标准。

**参考标准**:
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Qt Coding Conventions](https://wiki.qt.io/Coding_Conventions)
- [C++ Core Guidelines (NL section)](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#nl-naming-and-layout-suggestions)
- [ROS2 Code Style](https://docs.ros.org/en/rolling/The-ROS2-Project/Contributing/Code-Style-Language-Versions.html)

---

## 1. 目录命名

- **规范**: `snake_case`（全小写，下划线分隔）
- **理由**: 跨平台兼容（Linux 大小写敏感，Windows 不敏感），避免路径歧义
- **示例**:
  - `shared_memory/`, `motion_control/`, `behavior_tree/`
  - `src/`, `include/`, `modules/`, `resources/`

---

## 2. 文件命名

### 2.1 C++ 源码与头文件（`.cpp`, `.h`, `.hpp`）

- **规范**: `PascalCase`（大驼峰），文件名与核心类名**完全一致**
- **来源**: Google 推荐 `snake_case`，Qt 推荐 `PascalCase` 与类名一致。本项目采用 Qt 风格（PascalCase），因 GUI 占比大且类名→文件名的映射更直观
- **示例**:
  - `RobotModel.cpp` / `RobotModel.h` → 内含 `class RobotModel`
  - `MainWindow.cpp` / `MainWindow.h` → 内含 `class MainWindow`
- **特例**: 纯工具/函数文件（无核心类）使用 `snake_case`，如 `math_utils.h`, `rt_helpers.cpp`
- **禁止**: 文件名包含空格、中文或特殊字符

### 2.2 Qt 界面文件（`.ui`）

- **规范**: `snake_case`（全小写蛇形）
- **理由**: 遵循 Qt Designer 默认习惯
- **重要**: `#include` 生成的 UI 头文件时必须与 `.ui` 文件名严格对应
  - `main_window.ui` → `#include "ui_main_window.h"`
  - `alarm_panel.ui` → `#include "ui_alarm_panel.h"`
- **禁止**: `MainWindow.ui`、`alarmPanel.ui`

### 2.3 Qt 资源文件（`.qrc`）

- **规范**: `snake_case`，如 `icons.qrc`, `app_resources.qrc`

### 2.4 Protobuf 文件（`.proto`）

- **规范**: `snake_case`（遵循 Protobuf 官方风格指南）
- **示例**: `motion_command.proto`, `message.proto`
- **内部规范**:
  - 消息名: `PascalCase`（如 `MotionCommand`）
  - 字段名: `snake_case`（如 `joint_angles`, `max_velocity`）
  - 枚举值: `ALL_CAPS`（如 `MOVE_TYPE_JOINT = 0`）

### 2.5 CMake 文件

- `CMakeLists.txt`（固定名称）
- 模块文件: `snake_case.cmake`，如 `3rd_party.cmake`, `controller.cmake`

---

## 3. 头文件保护

- **规范**: 统一使用 `#pragma once`
- **理由**: 简洁、不易出错、所有主流编译器（GCC、Clang、MSVC、MinGW）均支持。避免 `#ifndef` 宏名与文件名不匹配的常见错误
- **示例**:
```cpp
#pragma once

#include <vector>

class RobotModel {
    // ...
};
```
- **禁止**: 新代码使用 `#ifndef XXX_H` / `#define XXX_H` 风格

---

## 4. 代码命名规范

### 4.1 总览表

| 实体 | 规范 | 示例 | 参考来源 |
|------|------|------|----------|
| **命名空间** | `snake_case` | `namespace motion_control` | Google, C++ Core Guidelines |
| **类 / 结构体** | `PascalCase` | `class RobotModel`, `struct WayPoint` | Google, Qt |
| **函数 / 方法** | `camelCase` | `void startMotion()` | Qt (Google 用 PascalCase, 本项目选 Qt 风格) |
| **局部变量** | `camelCase` | `double maxVelocity` | Qt |
| **函数参数** | `camelCase` | `void move(int axisId)` | Qt |
| **类成员变量** | `camelCase` + 后置 `_` | `double velocity_;` | Google (区分局部变量) |
| **静态成员变量** | `camelCase` + 后置 `_` | `static int instanceCount_;` | Google |
| **全��变量** | `g_` 前缀 + `camelCase` | `int g_debugLevel;` | Google (尽量避免全局变量) |
| **编译期常量** | `k` 前缀 + `PascalCase` | `constexpr int kMaxAxisCount = 6;` | Google |
| **宏常量** | `ALL_CAPS` | `#define MAX_BUFFER_SIZE 1024` | Google, 通用 |
| **枚举类型名** | `PascalCase` | `enum class MotionType` | Google, Qt |
| **枚举值 (enum class)** | `k` 前缀 + `PascalCase` | `kLinear`, `kJoint`, `kCircular` | Google (推荐), 类型安全 |
| **枚举值 (传统 enum)** | `ALL_CAPS` | `MOTION_LINEAR`, `MOTION_JOINT` | 传统 C++ 风格 |
| **模板参数** | `PascalCase` | `template<typename Element>` | Google, 通用 |
| **类型别名** | `PascalCase` | `using JointVector = std::vector<double>;` | Google |
| **概念 (C++20)** | `PascalCase` | `concept Movable` | C++ Core Guidelines |

### 4.2 详细说明

#### 命名空间
```cpp
// 正确
namespace zrcs {
namespace motion_control {
}  // namespace motion_control
}  // namespace zrcs

// 错误
namespace MotionControl {}  // PascalCase 不用于命名空间
namespace motionControl {}  // camelCase 不用于命名空间
```

#### 类与结构体
```cpp
// class 用于具有不变量或复杂行为的类型
class PathPreprocessor {
public:
    void processPath(const std::vector<WayPoint>& path);
private:
    double tolerance_;
};

// struct 用于纯数据聚合（所有成员 public，无不变量）
struct WayPoint {
    double x;
    double y;
    double z;      // struct 的成员不加后置下划线
};
```
- **struct vs class 成员变量约定**:
  - `class` 成员变量: 后置 `_`（如 `velocity_`）——因为有 private 访问控制，需与局部变量区分
  - `struct` 成员变量: **不加** 后置 `_`（如 `x`, `y`）——纯数据，全部 public，无歧义

#### 常量与枚举
```cpp
// constexpr / const 常量 —— k 前缀 + PascalCase
constexpr int kMaxAxisCount = 6;
constexpr double kDefaultVelocity = 100.0;
const std::string kConfigFileName = "robot.xml";

// 宏常量 —— ALL_CAPS（尽量用 constexpr 替代宏）
#define MAX_CMD_NAME 64

// 强类型枚举（推荐）—— k 前缀 + PascalCase
enum class MotionType {
    kLinear,
    kJoint,
    kCircular,
};

// 传统枚举（仅在需要隐式转换时使用）—— ALL_CAPS 带类型前缀
enum MotionMode {
    MOTION_MODE_AUTO,
    MOTION_MODE_MANUAL,
    MOTION_MODE_STEP,
};
```

#### 模板参数
```cpp
template<typename T>             // 单字母（简单泛型）
template<typename Element>       // PascalCase 描述性名称
template<size_t N>               // 非类型参数：小写或大写均可
template<typename InputIterator> // PascalCase
```

---

## 5. Qt 特定规范

### 5.1 信号与槽

| 类别 | 规范 | 示例 |
|------|------|------|
| **信号** | `camelCase`，过去分词或形容词描述状态变化 | `connected()`, `errorOccurred(int code)`, `dataReady()` |
| **公共槽** | `camelCase`，动词开头描述动作 | `sendCommand()`, `updateDisplay()` |
| **私有槽（响应 UI）** | `on_<objectName>_<signal>` 自动连接格式 | `on_startButton_clicked()` |
| **私有槽（手动连接）** | `camelCase`，动词开头 | `handleTimeout()`, `processReply()` |

```cpp
class ZmqClient : public QObject {
    Q_OBJECT
signals:
    void connected();
    void messageReceived(const QByteArray& data);
    void errorOccurred(const QString& message);

public slots:
    void connectToServer(const QString& host, int port);
    void disconnect();

private slots:
    void handleSocketReady();
    void on_retryButton_clicked();  // 仅 auto-connection 时使用此格式
};
```

### 5.2 Q_PROPERTY

```cpp
// 属性名: camelCase
// READ/WRITE 方法遵循 Qt 惯例
Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
Q_PROPERTY(double velocity READ velocity WRITE setVelocity NOTIFY velocityChanged)
```

- 布尔属性的 getter: `is` 前缀（如 `isConnected()`）
- 普通属性的 getter: 属性名本身（如 `velocity()`）
- setter: `set` 前缀（如 `setVelocity(double v)`）

### 5.3 UI 文件与代码的对应

| UI 文件名 | 生成头文件 | `#include` 写法 |
|-----------|-----------|----------------|
| `main_window.ui` | `ui_main_window.h` | `#include "ui_main_window.h"` |
| `alarm_panel.ui` | `ui_alarm_panel.h` | `#include "ui_alarm_panel.h"` |

---

## 6. Include 顺序

遵循 Google C++ Style Guide 的 include 分组和排序规则，每组之间空一行：

```cpp
// 1. 本文件对应的头文件（验证头文件自包含）
#include "core/MainWindow.h"

// 2. C 系统头文件
#include <cstdint>
#include <cstring>

// 3. C++ 标准库头文件
#include <memory>
#include <string>
#include <vector>

// 4. 第三方库头文件
#include <QWidget>
#include <zmq.hpp>
#include <Eigen/Dense>

// 5. 本项目其他头文件
#include "communication/ZmqClient.h"
#include "config/ZrcsConfig.h"
```

每组内按字母顺序排列。

---

## 7. 注释规范

### 7.1 文件头注释

每个源文件/头文件顶部应包含简要说明（不强制版权声明模板，但鼓励）：

```cpp
/// @file RobotModel.h
/// @brief 机器人运动学模型定义
```

### 7.2 函数与类注释

- 在 **头文件** 中的声明处写 Doxygen 风格文档注释
- 在 `.cpp` 实现处只写实现细节注释，不重复接口文档

```cpp
/// @brief 执行关节空间点到点运动
/// @param target 目标关节角度（弧度）
/// @param velocity 运动速度比例 [0.0, 1.0]
/// @return true 若命令已入队
bool moveJoint(const JointVector& target, double velocity);
```

### 7.3 行内注释

```cpp
double ratio = override / 100.0;  // 将百分比转换为 [0,1] 比例
```

- TODO 注释格式: `// TODO(username): description`
- FIXME 注释格式: `// FIXME(username): description`

---

## 8. 其他规范

### 8.1 大括号风格

采用 **Allman 风格**（开大括号单独占一行），以 `NodeManager.cpp` 为基准：

```cpp
// 控制语句：开大括号换行
for (auto& node : factory_.inPutNodes)
{
    node->registered(controller_.get(), rtProcess_.get());
}

if (condition)
{
    doSomething();
}
else
{
    doOther();
}

switch (value)
{
    case kFoo:
        break;
    default:
        break;
}

// 函数/方法定义：开大括号换行
void NodeManager::run()
{
    // ...
}

// Lambda：开大括号换行
controller_->rtos_->real_task([this]()
{
    // ...
});

// 类/结构体定义：开大括号同行（例外）
class MyClass {
public:
    // ...
};
```

- 控制语句（`if`/`else`/`for`/`while`/`switch`）: 开大括号**换行**
- 函数/方法定义: 开大括号**换行**
- Lambda 表达式: 开大括号**换行**
- 类/结构体定义: 开大括号**同行**（唯一例外）

### 8.2 缩进与空白

- 缩进: **4 个空格**（不使用 Tab）
- 行宽: 不超过 **120 字符**
- 指针与引用: 靠近类型 `int* ptr`（Qt 风格）而非 `int *ptr`

### 8.3 CMake 目标命名

- 与目录名一致，使用去下划线的小写形式: 目录 `zrcs_rt/` → 目标 `zrcsrt`
- 库目标使用描述性名称: `behavior_tree_editor`, `motion_planner`

---

## 9. 检查清单

AI 进行 Code Review、新建文件或重构时，**必须**逐项核查：

### 文件层面
- [ ] 文件名不含空格、中文或特殊字符
- [ ] `.cpp`/`.h` 文件使用 `PascalCase`，且与核心类名一致
- [ ] `.ui`/`.qrc` 文件使用 `snake_case`
- [ ] `.proto` 文件使用 `snake_case`
- [ ] 目录名使用 `snake_case`
- [ ] 头文件使用 `#pragma once`（非 `#ifndef` 宏守卫）
- [ ] Include 顺序正确（本文件头 → C → C++ → 第三方 → 项目）

### 代码层面
- [ ] 类/结构体名为 `PascalCase`
- [ ] 函数/方法名为 `camelCase`
- [ ] `class` 成员变量带后置 `_`；`struct` 纯数据成员不带
- [ ] 命名空间为 `snake_case`
- [ ] `constexpr`/`const` 常量使用 `kPascalCase`
- [ ] `enum class` 值使用 `kPascalCase`；传统 `enum` 值使用 `ALL_CAPS`
- [ ] 模板参数为 `PascalCase`
- [ ] 避免全局变量；若必须使用则加 `g_` 前缀

### Qt 层面
- [ ] 信号名为过去分词/形容词（`connected`, `dataReady`）
- [ ] 槽名为动词开头（`sendCommand`, `handleReply`）
- [ ] `#include "ui_xxx.h"` 与 `.ui` 文件名严格对应
- [ ] 布尔属性 getter 使用 `is` 前缀

# CMake 构建说明

## 概述

ZRCS 项目使用 CMake 构建系统，支持多种构建模式（标准、实时、仿真），通过模块化的 CMake 配置文件组织编译流程。

## CMake 模块结构

项目的 CMake 配置分布在以下文件中：

```
zrcs-dev/
  CMakeLists.txt              # 根构建文件
  cmake/
    controller.cmake          # 构建模式控制（realtime/simulation/standard）
    3rdParty.cmake             # 第三方库依赖配置
    tool.cmake                 # 工具和脚本配置
  zrcsGui/CMakeLists.txt       # GUI 子项目
  zrcsNrt/CMakeLists.txt       # NRT 子项目
  zrcsRt/CMakeLists.txt        # RT 子项目
```

## 模块说明

### controller.cmake

控制构建模式选择，定义三种模式：

| 模式 | 宏定义 | 说明 |
|------|--------|------|
| `standard` | `-DSTANDARD` | 标准模式，默认选项 |
| `realtime` | `-DREALTIME -DETHERCAT` | 实时模式，启用 Xenomai 和 EtherCAT |
| `simulation` | `-DSIMULATION` | 仿真模式，启用 CoppeliaSim 支持 |

实时模式下会自动调用 `xeno-config` 获取 Xenomai 编译和链接参数。仿真模式下会添加 CoppeliaSim Remote API 的头文件和库路径。

### 3rdParty.cmake

管理所有第三方库依赖：

- **tinyxml2** -- XML 解析库（以子目录方式编译）
- **ruckig** -- 在线轨迹生成库（以子目录方式编译）
- **Boost** -- 进程间通信（共享内存）
- **Protobuf** -- 消息序列化
- **Abseil (absl)** -- 日志和工具库（Protobuf 依赖）
- **Threads** -- 系统线程库

所有第三方库强制编译为静态库（`BUILD_SHARED_LIBS OFF`）。

### tool.cmake

查找 Python3 解释器，并注册自定义构建目标 `run_python_scripts`，在构建时自动执行 `tool/config.py` 配置脚本。

## 构建选项

| 选项 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `BUILD_MODE` | STRING | `standard` | 构建模式：realtime / simulation / standard |
| `test` | BOOL | `ON` | 是否编译测试程序 |

## 构建示例

### 标准模式（默认）

```bash
cd zrcs-dev
mkdir build && cd build
cmake ..
make
# 或 Windows 下：
cmake --build .
```

### 实时模式（Linux + Xenomai）

```bash
cd zrcs-dev
mkdir build && cd build
cmake -DBUILD_MODE=realtime ..
make
```

需要系统已安装 Xenomai 实时框架（路径：`/usr/xenomai/`）和 EtherCAT 主站库。

### 仿真模式

```bash
cd zrcs-dev
mkdir build && cd build
cmake -DBUILD_MODE=simulation ..
make
```

需要系统已安装 CoppeliaSim Edu。

### Windows 构建（MinGW）

```bash
cd zrcs-dev
mkdir build && cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

## 构建目标

| 目标 | 说明 |
|------|------|
| `zrcsgui` | Qt GUI 应用程序 |
| `zrcsnrt` | 非实时控制进程 |
| `zrcsrt` | 实时控制进程 |
| `run_python_scripts` | 执行 Python 配置脚本 |

所有可执行文件输出到 `build/bin/` 目录。

## 必需依赖

| 依赖 | 用途 | 安装方式（Ubuntu） |
|------|------|---------------------|
| CMake >= 3.18.1 | 构建系统 | `apt install cmake` |
| C++17 编译器 | 编译 | `apt install g++` |
| Qt5 | GUI 框架 | `apt install qtbase5-dev` |
| Boost | 共享内存 | `apt install libboost-dev` |
| Protobuf | 消息序列化 | `apt install libprotobuf-dev protobuf-compiler` |
| Abseil | 日志/工具 | `apt install libabsl-dev` |
| Python3 | 配置脚本 | `apt install python3` |

## 可选依赖

| 依赖 | 构建模式 | 说明 |
|------|----------|------|
| Xenomai | realtime | 实时框架，需从源码安装 |
| EtherCAT 主站 | realtime | `ethercat_rtdm` 库 |
| CoppeliaSim | simulation | 机器人仿真软件 |
| ZMQ / cppzmq | 所有 | ZMQ 通信（`apt install libzmq3-dev libcppzmq-dev`） |

## 核心链接库

根 CMakeLists.txt 定义了公共链接库集合 `MOTION_CONTROL_LIBS`：

```
Threads::Threads
ruckig
tinyxml2
atomic
protobuf::libprotobuf
absl::log_internal_message
absl::log_internal_check_op
absl::log_globals
```

实时模式额外链接 `ethercat_rtdm`，仿真模式额外链接 `remoteApi`。

## 故障排查

### Xenomai 未找到

**现象**：`/usr/xenomai/bin/xeno-config: No such file or directory`

**解决**：确认 Xenomai 已安装到 `/usr/xenomai/`，或修改 `controller.cmake` 中的 `xeno-config` 路径。

### EtherCAT 库未找到

**现象**：链接时报 `cannot find -lethercat_rtdm`

**解决**：安装 EtherCAT 主站库，或切换到 `standard` 模式。

### CoppeliaSim 路径错误

**现象**：仿真模式下找不到 CoppeliaSim 头文件

**解决**：修改 `controller.cmake` 中 CoppeliaSim 的安装路径，使其匹配本地实际安装位置。当前硬编码路径：
- Windows: `C:/Program Files/CoppeliaRobotics/CoppeliaSimEdu/`
- Linux: `/home/zrcs/Downloads/CoppeliaSim_Edu_V4_10_0_rev0_Ubuntu24_04/`

### Protobuf 版本不兼容

**现象**：编译时出现 Protobuf 相关错误

**解决**：确保 Protobuf 和 Abseil 版本匹配。推荐通过包管理器统一安装，或使用 vcpkg 管理。

### Qt5 未找到

**现象**：`Could not find a package configuration file provided by "Qt5"`

**解决**：
- Ubuntu: `apt install qtbase5-dev qt5-qmake`
- Windows: 安装 Qt5 并设置 `CMAKE_PREFIX_PATH` 指向 Qt 安装目录

---

版本: 1.0
最后更新: 2026-03-18

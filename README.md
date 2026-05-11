
# ZRCS

ZRCS（Zhang Real-time Control System）是一个面向运动控制场景的 C++ 工程，采用“GUI / NRT / RT / Common”分层架构，支持标准模式、仿真模式和实时模式，覆盖共享内存通信、ZMQ 通信、机器人模型、轨迹命令、EtherCAT/虚拟控制器接入以及 Qt 上位机界面。

当前仓库更像一套完整的运动控制软件工作台，而不只是单一的控制内核。它包含：

- `zrcs_rt`：实时控制进程，负责控制循环、命令执行、硬件驱动与模型调用
- `zrcs_nrt`：非实时进程，负责桥接 RT、对外提供命令接口、发布状态、消费 RT 日志
- `zrcs_common`：共享代码，包括共享内存结构、项目配置、XML 解析、命令定义等
- `zrcs_gui`：主 Qt 图形界面，面向综合控制、状态展示和扩展功能
- `motionGui`：轻量级运动调试界面
- `config/`：按项目拆分的轴、模型、EtherCAT 等配置
- `3rdParty/`：内置第三方依赖，如 `ruckig`、`tinyxml2`、`spdlog`、`cppzmq`、`abseil-cpp`、`BehaviorTree.CPP` 等

## 主要特性

- 多进程分层：GUI、NRT、RT 解耦，适合将非实时逻辑与实时控制隔离
- 共享内存桥接：NRT 和 RT 通过无锁共享内存结构交换命令、日志、反馈和配置
- 网络通信：GUI 与 NRT 通过 ZMQ + Protobuf 通信
- 轨迹控制：支持 `MoveAbs`、`MoveAbsJ`、`MoveJ`、`MoveL`、`MoveC`、`Movehome`、`MoveLGalvo`、`JogJ`、`JogabsJ`、连续点动等命令
- 模型支持：包含串联机器人、并联机器人、笛卡尔机器人模型与 FK/IK 能力
- 多构建模式：支持 `standard`、`simulation`、`realtime`
- 多界面入口：提供主 GUI 与轻量调试 GUI
- 项目化配置：通过 `config/project.txt` 切换当前机型/项目
- Windows 打包：仓库内已经提供安装包脚本

## 工程结构

```text
zrcs-dev/
|- zrcs_common/      公共库：共享内存、配置、XML、命令定义
|- zrcs_nrt/         非实时进程：ZMQ、状态发布、RT 桥接、终端控制
|- zrcs_rt/          实时进程：控制器、模型、命令、调度器
|- zrcs_gui/         主 Qt GUI
|- motionGui/        轻量级运动控制 GUI
|- config/           项目配置（3axis、5axis、demo、ur5 等）
|- docs/             补充文档
|- test/             测试源码
|- 3rdParty/         第三方依赖
|- tool/             打包与辅助工具
`- CMakeLists.txt    顶层构建入口
```

## 模块说明

### `zrcs_common`

公共基础库，主要负责：

- 共享内存布局定义
- 项目配置解析
- XML 解析
- 命令 ID 与参数定义
- NRT/RT 进程共享的数据结构

典型内容：

- `shared_memory/ShmLayout.h`
- `shared_memory/NrtProcess.h`
- `shared_memory/RtProcess.h`
- `config/ProjectConfig.h`
- `config/CmdDefine.h`
- `xml/XmlParsing.h`

### `zrcs_nrt`

非实时主进程，通常是系统实际启动的后台主入口。主要负责：

- 初始化共享内存
- 拉起 `zrcsrt` 子进程
- 启动 ZMQ 命令服务
- 启动 ZMQ 状态发布器
- 提供 RT 桥接接口
- 消费 RT 日志
- 提供命令行终端入口

### `zrcs_rt`

实时控制进程，负责：

- 读取当前项目配置
- 初始化控制器、模型和节点工厂
- 启动实时循环
- 周期性执行输入节点、命令节点、输出节点
- 向控制器写入位置/速度/IO 指令
- 向共享内存写回心跳、状态和日志

### `zrcs_gui`

Qt6 主界面，负责：

- 连接 NRT 命令服务和状态发布服务
- 展示系统状态、轴状态、日志、报警
- 提供命令面板、点动控制、IO 控制、轨迹可视化、GCode、远程监控、插件等功能

### `motionGui`

更加轻量，适合快速调试运动系统，重点覆盖：

- 连接管理
- 轴使能/失能
- 回零/复位
- 点动/Jog
- 速度倍率
- 基本状态展示

## 整体架构

ZRCS 采用三层通信链路：

1. GUI -> NRT：ZMQ `REQ/REP` 发送命令
2. NRT -> GUI：ZMQ `PUB/SUB` 发布状态
3. NRT <-> RT：共享内存交换命令、日志、反馈和配置

可简化理解为：

```text
zrcsgui / motiongui
        |
        |  ZMQ + Protobuf
        v
      zrcsnrt
        |
        |  Shared Memory (lock-free queues + atomics + latest values)
        v
      zrcsrt
        |
        |  Controller / Model / EtherCAT / Virtual Servo
        v
      Hardware / Simulation
```

## 通信机制

### 1. GUI <-> NRT

- 命令端口：`5555`
- 状态端口：`5556`
- 命令协议：`MotionCommand` protobuf
- 状态协议：`SystemStatus` protobuf

GUI 侧通过 ZMQ 客户端发送命令到 NRT；NRT 收到命令后，决定是直接处理系统命令，还是将普通命令转发到 RT。

### 2. NRT <-> RT

NRT 与 RT 之间使用共享内存，核心结构定义在 `zrcs_common/shared_memory/ShmLayout.h`。

共享内存中包含：

- `cmdQueue`：NRT -> RT 命令队列
- `logQueue`：RT -> NRT 日志队列
- `axisFeedbackQueue`：RT -> NRT 高频轴反馈
- `taskSched`：系统调度状态
- `lastCmdSeq / lastCmdResult`：命令完成跟踪
- `axisPositions / fkResult / heartbeat`：最新值通道
- `pathMoveCfg / galvoCfg`：路径运动与振镜配置

### 3. RT 主循环

RT 侧主调度器是 `NodeManager`，核心循环大致为：

```text
receiveData
-> InputNodes
-> CmdNode
-> OutputNodes
-> sendData
-> heartbeat update
```

同时根据 `taskSched` 管理 `RUN / STOP / RESET / ERROR_STATE / SHUTDOWN` 等全局状态。

## 命令链路

普通命令的执行流程如下：

1. GUI 发送 `MotionCommand(command, args[])`
2. `zrcsnrt` 中的 `ZMQServer` 接收并解析消息
3. `RtBridge` 将命令名转换为 `CmdId`
4. 命令被封装为共享内存中的 `Command`
5. RT 从 `cmdQueue` 取出命令
6. `NodeFactory` 根据 `CmdId` 找到对应命令节点
7. RT 执行 `init() / run() / exit()` 生命周期
8. RT 回写命令完成状态

当前已落地的 RT 命令主要包括：

- `Enable`
- `Disable`
- `Reset`
- `Setmode`
- `SetZero`
- `MoveAbs`
- `MoveAbsJ`
- `MoveJ`
- `MoveL`
- `MoveC`
- `Movehome`
- `MoveLGalvo`
- `JogJ`
- `JogabsJ`
- `ContinuousJog`
- `DataPub`

如果需要新增命令，可以参考：

- `docs/add_command_guide.md`

## 运动与控制能力概览

当前工程已具备的核心运动控制能力包括：

- 单轴绝对运动、关节绝对运动
- 笛卡尔空间 `MoveJ`
- 笛卡尔直线插补 `MoveL`
- 圆弧插补 `MoveC`
- 回零 `Movehome`
- 连续点动 `ContinuousJog`
- 平台 + 振镜联动 `MoveLGalvo`
- Ruckig jerk-limited 轨迹生成
- 基本轴状态机、软限位、方向限制、多驱同步误差检查
- EtherCAT 和虚拟伺服双形态控制器抽象

相关运动审查文档可参考：

- `docs/motion-review.md`

## 项目配置体系

ZRCS 采用项目化配置方式。当前激活项目由 `config/project.txt` 指定。

当前仓库内该文件内容为：

```text
3axis
```

这意味着系统默认会从 `config/3axis/` 目录中读取相关配置文件。

典型项目配置包括：

- `axis.xml`：轴参数、限位、伺服配置
- `model.xml`：机器人模型参数
- `ethercat.xml`：EtherCAT 从站配置
- `io.xml`：部分项目中用于 IO 定义
- `laser.xml`：激光相关配置（特定项目）

已有示例项目包括：

- `3axis`
- `5axis`
- `demo`
- `single-axis`
- `ur5`
- `hg-5axis`
- `galvo-platform`

切换项目时，通常只需修改：

- `config/project.txt`

## 构建模式

顶层 CMake 当前使用 `BUILD_MODE` 控制构建形态：

- `standard`：标准模式，默认模式
- `simulation`：仿真模式，接入 CoppeliaSim Remote API
- `realtime`：实时模式，接入 Xenomai/EtherCAT

注意：这比旧 README 中的 `-Drealtime=YES -Dethercat=YES` 更接近当前代码的真实构建方式。旧写法可以视为历史用法说明，不建议继续作为主文档命令使用。

## 环境依赖

### Linux 依赖

原 README 中已有这些依赖说明，保留并合并如下：

```bash
sudo apt install libzmq3-dev
sudo apt install nlohmann-json3-dev
sudo apt install libeigen3-dev
sudo apt install libabsl-dev
sudo apt install qt6-multimedia-dev
sudo apt install libqt6svg6-dev qt6-base-dev
sudo apt install libncurses-dev libncursesw5-dev
```

此外，当前 CMake 实际还要求：

- CMake 3.18.1+
- 支持 C++17 的编译器
- Protobuf
- Boost
- Threads
- Eigen3

实时模式还需要：

- Xenomai
- `ethercat_rtdm`

### Windows / MSYS2 依赖

原 README 中已有的 MSYS2 依赖：

```bash
pacman -S mingw-w64-ucrt-x86_64-nlohmann-json
pacman -S mingw-w64-ucrt-x86_64-zeromq
pacman -S mingw-w64-ucrt-x86_64-cppzmq
pacman -S mingw-w64-ucrt-x86_64-protobuf
pacman -S mingw-w64-ucrt-x86_64-opencascade
pacman -S mingw-w64-ucrt-x86_64-eigen3
```

如果需要构建 GUI 和打包，建议再安装：

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base
pacman -S mingw-w64-ucrt-x86_64-qt6-svg
pacman -S mingw-w64-ucrt-x86_64-qt6-multimedia
pacman -S mingw-w64-ucrt-x86_64-abseil-cpp
pacman -S mingw-w64-ucrt-x86_64-nsis
```

## 快速开始

### 1. 克隆工程

```bash
git clone <your-repo-url> zrcs-dev
cd zrcs-dev
```

### 2. 标准模式构建

```bash
cmake -S . -B build -DBUILD_MODE=standard
cmake --build build
```

### 3. 仿真模式构建

```bash
cmake -S . -B build -DBUILD_MODE=simulation
cmake --build build
```

### 4. 实时模式构建

```bash
cmake -S . -B build -DBUILD_MODE=realtime
cmake --build build
```

### 5. 构建产物

默认可执行文件输出到：

- `build/bin/zrcsnrt`
- `build/bin/zrcsrt`
- `build/bin/zrcsgui`
- `build/bin/motiongui`

说明：

- `realtime` 模式下通常只构建 `zrcsnrt` 和 `zrcsrt`
- 非 `realtime` 模式下还会构建 `zrcsgui` 和 `motiongui`

## 运行方式

### 推荐启动顺序

1. 启动 `zrcsnrt`
2. 启动 `zrcsgui`
3. 如果需要，也可以启动 `motiongui`

通常后端只需要手动启动 `zrcsnrt`，因为它会自动拉起 `zrcsrt` 子进程。

### Linux 端口开放

原 README 中已有说明，保留如下：

```bash
sudo ufw allow 5555
sudo ufw allow 5556
```

### 典型运行示例

Linux / MSYS2 / PowerShell 下都可以按产物路径直接运行，例如：

```bash
./build/bin/zrcsnrt
./build/bin/zrcsgui
```

在 Windows PowerShell 中：

```powershell
.\build\bin\zrcsnrt.exe
.\build\bin\zrcsgui.exe
```

## GUI 与工具程序

### `zrcsgui`

主 GUI，适合综合操作与状态监控，包含：

- 主状态面板
- 点动与 IO 面板
- 报警面板
- 命令面板
- GCode 编辑
- 轨迹可视化
- 插件管理
- 远程监控

### `motiongui`

更轻量，适合本地快速调试与单机验证，包含：

- 连接控制
- 各轴基本状态展示
- Enable / Disable / Reset / ErrorClear
- Jog
- Home
- 速度倍率调整

## 打包

原 README 中已经给出 Windows 安装包打包说明，现整理如下。

### 打包脚本

仓库中提供：

- `tool/package/package_installer.sh`

该脚本会自动：

- 复制 `build/bin/zrcsgui.exe`
- 附带复制 `motiongui.exe`、`zrcsnrt.exe`、`zrcsrt.exe`
- 使用 `windeployqt6` 收集 Qt 依赖
- 使用 `ldd` 递归补齐 MSYS2 运行时 DLL
- 生成 NSIS 安装包 `.exe`

### 打包前先编译

```powershell
cmake --build build --target zrcsgui --config Release
cmake --build build --target motiongui --config Release
cmake --build build --target zrcsnrt --config Release
cmake --build build --target zrcsrt --config Release
```

### 运行打包脚本

方式一：PowerShell 调用 MSYS2 bash

```powershell
D:\msys2\usr\bin\bash.exe tool/package/package_installer.sh
```

如果 MSYS2 安装在 `C:\msys2`：

```powershell
C:\msys2\usr\bin\bash.exe tool/package/package_installer.sh
```

方式二：在 Git Bash 或 MSYS2 bash 中运行

```bash
bash tool/package/package_installer.sh
```

或：

```bash
cd tool/package
bash ./package_installer.sh
```

### 打包输出

- 安装包：`build/ZRCS-2.0.0-Setup.exe`
- 中间目录：`build/installer_staging/`

## 测试

仓库中存在单独的测试源码目录：

- `test/`

当前可见的测试目标包括：

- `parameter`
- `test_spsc`
- `test_command`
- `test_zmq_comm`

需要注意的是：

- 当前顶层 `CMakeLists.txt` 没有默认 `add_subdirectory(test)`
- 因此这些测试不会自动加入主构建流程
- 如果你想启用统一测试构建，需要额外把 `test/` 接入顶层 CMake

## 常见开发入口

如果你准备继续开发，建议优先阅读这些文件：

- 顶层构建入口：`CMakeLists.txt`
- 项目配置选择：`config/project.txt`
- 共享内存 ABI：`zrcs_common/shared_memory/ShmLayout.h`
- 命令定义：`zrcs_common/config/CmdDefine.h`
- RT 调度器：`zrcs_rt/system/NodeManager.cpp`
- 控制器抽象：`zrcs_rt/controller/ControllerInterface.h`
- RT 命令汇总：`zrcs_rt/command/CmdHead.h`
- NRT 主程序：`zrcs_nrt/main.cpp`
- ZMQ 服务端：`zrcs_nrt/zmq_server/ZmqServer.h`
- 状态发布器：`zrcs_nrt/statusPublisher/StatusPublisher.h`
- 主 GUI 入口：`zrcs_gui/core/main.cpp`

## 文档索引

仓库中已有几份值得优先阅读的文档：

- `docs/add_command_guide.md`：新增命令的完整链路说明
- `docs/motion-review.md`：路径预处理与 RT 运动链路审查

## 常见问题

### 1. 为什么推荐先启动 `zrcsnrt`？

因为 `zrcsnrt` 负责创建共享内存、启动 ZMQ 服务，并自动拉起 `zrcsrt`。如果直接运行 GUI 而后端没启动，界面只能进入等待连接状态。

### 2. 为什么旧 README 里的构建命令和现在不一样？

旧文档使用的是历史风格参数，例如：

```bash
cmake -Drealtime=YES -Ddebug=YES -Dethercat=YES ..
```

当前代码已经统一采用：

```bash
cmake -S . -B build -DBUILD_MODE=standard
cmake -S . -B build -DBUILD_MODE=simulation
cmake -S . -B build -DBUILD_MODE=realtime
```

如果没有兼容包装层，优先以 `BUILD_MODE` 为准。

### 3. 提示找不到 `zrcsgui.exe`

说明 GUI 还没有编译成功，先执行：

```powershell
cmake --build build --target zrcsgui --config Release
```

### 4. 提示找不到 `windeployqt6`

说明 Qt6 基础包没有安装完整，执行：

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base
```

### 5. 提示找不到 `makensis`

说明 NSIS 未安装，执行：

```bash
pacman -S mingw-w64-ucrt-x86_64-nsis
```

### 6. 安装包里默认包含哪些程序？

当前默认包含：

- `zrcsgui.exe`
- `motiongui.exe`
- `zrcsnrt.exe`
- `zrcsrt.exe`

如果需要增加额外可执行文件，可以修改：

- `tool/package/package_installer.sh`

## 备注

- 当前默认项目是 `3axis`
- 如果你在 Windows 上开发，建议优先使用 MSYS2 UCRT64 环境
- 如果你在 Linux 上进行实时控制部署，需要额外准备 Xenomai、EtherCAT 和实时运行环境
- 如果你的目标是继续扩展命令链路，优先从 `CmdDefine.h`、`CmdHead.h`、`NodeManager.cpp` 和 `RtBridge.h` 入手

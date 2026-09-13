# 如何新增一个命令

本文说明在当前 `zrcs` 工程里，新增一个命令时需要在哪些地方加代码，以及命令如何从上位机一路传到 RT 执行。

适用范围：

- RT 命令节点，位于 `zrcs_rt/command`
- NRT 转发链路，位于 `zrcs_nrt`
- GUI/上位机发送入口，位于 `zrcs_gui`

- `zrcs_rt/system/CmdMeta.h`
- `zrcs_rt/system/CmdIds.h`
- `zrcs_rt/command/CmdHead.h`

## 1. 命令整体链路

新增命令后，完整调用链如下：

1. GUI 通过 `ZMQClient` 发送 `MotionCommand`
2. NRT 的 `ZMQServer` 在 `5555` 端口接收命令
3. `ZMQServer` 调用 `RtBridge::sendCommand()`
4. `RtBridge` 根据命令名在 `cmdNameToId()` 中查找 `CmdId`
5. NRT 把命令写入共享内存 `cmdQueue`
6. RT 的 `NodeManager` 从 `cmdQueue` 取出命令
7. `NodeFactory` 根据 `CmdId` 找到对应的 `CmdNode`
8. RT 执行该命令的 `init() / run() / exit()`

对应代码位置：

- GUI 发送：`zrcs_gui/communication/ZmqClient.cpp`
- GUI 面板按钮：`zrcs_gui/modules/command/CommandPanel.cpp`
- NRT 接收与路由：`zrcs_nrt/zmq_server/ZmqServer.h`
- NRT 到 RT 桥接：`zrcs_nrt/rt_bridge/RtBridge.h`
- RT 调度执行：`zrcs_rt/system/NodeManager.cpp`
- RT 命令工厂注册：`zrcs_rt/system/NodeFactory.h`

## 2. 第一步：在 RT 新建命令类

在 `zrcs_rt/command` 下新增一对文件，例如：

- `zrcs_rt/command/MyCommand.h`
- `zrcs_rt/command/MyCommand.cpp`

建议参考已有命令：

- `zrcs_rt/command/SetZero.h`
- `zrcs_rt/command/SetZero.cpp`

### 2.1 头文件写法

头文件里需要：

- 继承 `zrcsSystem::CmdNode`
- 在 `public` 区域使用 `CMD_DEFINE(...)`
- 声明 `init()`、`run()`、`exit()`
- 在构造函数中设置 `nodeName_`

示例：

```cpp
#pragma once

#include "config/CmdArgs.h"
#include "system/base/BaseNodeInterface.h"
#include "system/CmdMeta.h"

class MyCommand : public zrcsSystem::CmdNode
{
public:
    CMD_DEFINE(60, PARAM(AxisId) PARAM(Value))

    MyCommand()
    {
        std::strcpy(nodeName_, "MyCommand");
    }

    void init() override;
    void run() override;
    void exit() override;

private:
    int axisId_{0};
    double value_{0.0};
};
```

说明：

- `CMD_DEFINE(60, ...)` 里的 `60` 是命令 ID，必须全局唯一
- `PARAM(AxisId)`、`PARAM(Value)` 会展开成参数下标枚举，可直接用于 `command_->args[AxisId]`

### 2.2 源文件写法

`.cpp` 中实现命令逻辑，并在文件底部使用 `REGISTERCMD(MyCommand)` 注册到 `NodeFactory`

示例：

```cpp
#include "command/MyCommand.h"

void MyCommand::init()
{
    axisId_ = static_cast<int>(command_->args[AxisId]);
    value_ = command_->args[Value];
}

void MyCommand::run()
{
    // 在这里写实际执行逻辑
    setCmdStatus(zrcsSystem::CmdStatus::EXIT);
}

void MyCommand::exit()
{
}

REGISTERCMD(MyCommand);
```

说明：

- `init()` 负责读取参数
- `run()` 负责执行核心逻辑
- 成功时通常把状态推进到 `EXIT`
- 失败时使用 `setCmdStatus(zrcsSystem::CmdStatus::FAILED);`
- 可用 `INFO_PRINT / WARN_PRINT / ERROR_PRINT` 输出 RT 日志

## 3. 第二步：把命令加入 RT 命令表

仅新增 `.h/.cpp` 还不够，当前项目还需要手工维护两处命令表。

### 3.1 更新 `CmdIds.h`

文件：

- `zrcs_rt/system/CmdIds.h`

这里需要同步改 3 个地方：

1. `enum class CmdId` 增加枚举项
2. `cmdIdToName()` 的 `kTable` 增加名称
3. `cmdNameToId()` 的 `kMap` 增加映射

例如增加：

```cpp
MyCommand = 60,
```

并同步：

```cpp
"MyCommand",    // 60
```

以及：

```cpp
{"MyCommand", CmdId::MyCommand},
```

最后别忘了更新：

```cpp
SENTINEL = 61
```

要求：

- `CmdId` 必须唯一
- `SENTINEL` 必须始终等于最大 ID + 1
- 名字必须和 GUI/NRT 发送时使用的字符串完全一致

### 3.2 更新 `CmdHead.h`

文件：

- `zrcs_rt/command/CmdHead.h`

在“已实现的命令”区域增加头文件：

```cpp
#include "command/MyCommand.h"
```

为什么这一步必须做：

- `NodeManager.cpp` 会包含 `command/CmdHead.h`
- 该头文件的作用之一是触发各命令 `REGISTERCMD(...)` 的静态注册
- 如果没加到这里，RT 运行时就可能报：

```text
未注册的命令: MyCommand(seq=xx), 已忽略
```

## 4. 第三步：确认 RT 调度是否需要特殊处理

大多数普通命令只需要走默认命令调度，不需要改 `NodeManager.cpp`。

文件：

- `zrcs_rt/system/NodeManager.cpp`

默认流程已经会：

- 从 `cmdQueue` 取命令
- 按 `CmdId` 通过 `factory_.getNodePtr()` 找到节点
- 调用 `registered(...)`
- 调用 `execute()`

只有以下情况才需要额外改 RT 调度层：

- 该命令不是普通 `CmdNode`，而是 `InputNode` / `OutputNode`
- 需要走持续运行型节点，不是一次性命令
- 需要新增共享内存字段
- 需要和 `TaskScheduling` 或错误恢复逻辑联动

## 5. 第四步：NRT 通信层通常是否需要改

通常情况下，普通命令不需要改 NRT 通信协议，只要命令名和参数列表确定即可。

关键文件：

- `zrcs_nrt/zmq_server/ZmqServer.h`
- `zrcs_nrt/rt_bridge/RtBridge.h`

### 5.1 普通命令

如果你的命令只是类似：

- `SetZero`
- `Enable`
- `MoveAbs`

这种“命令名 + 参数列表”的普通命令，那么一般不需要改 `ZMQServer`

原因：

- GUI 发来的 `MotionCommand.command`
- 会在 `ZMQServer::handleMotionCommand()` 里落到默认分支
- 默认分支会调用 `bridge_->sendCommand(name, args)`
- `RtBridge` 会在 `cmdNameToId()` 中查表，然后写入共享内存队列

也就是说，只要：

- `CmdIds.h` 里有名字到 ID 的映射
- RT 侧有命令实现并注册

NRT 就能自动转发。

### 5.2 特殊系统命令

如果命令属于系统控制类，而不是普通 RT 命令，比如：

- `SYS_RUN`
- `SYS_STOP`
- `SYS_RESET`
- `SYS_ESTOP`
- `SYS_JOG_START`

那么要在：

- `zrcs_nrt/zmq_server/ZmqServer.h`

的 `handleMotionCommand()` 里添加专门分支。

因为这些命令并不是直接映射到普通 `CmdNode`，而是会调用：

- `requestRun()`
- `requestStop()`
- `startContinuousMotion()`
- `setSpeedMultiplier()`

这类桥接接口。

### 5.3 需要新增桥接能力时

如果你的命令不是简单写入 `cmdQueue`，而是要：

- 写共享内存配置字段
- 写路径点队列
- 控制某个持续状态
- 读取 RT 结果

那么还要修改：

- `zrcs_nrt/rt_bridge/RtBridge.h`

典型例子：

- `setPathMoveConfig()`
- `setGalvoConfig()`
- `startContinuousMotion()`
- `waitForCompletion()`

## 6. 第五步：上位机 GUI 需要改哪些地方

这取决于你希望上位机如何发这个命令。

### 6.1 如果只想通过“通用命令输入框”测试

文件：

- `zrcs_gui/modules/command/CommandPanel.cpp`

现有“通用命令”区域已经支持直接输入：

- 命令名
- 逗号分隔参数

例如：

```text
命令名: MyCommand
参数: 0,123.4
```

这种情况下，GUI 通常不需要额外改代码。

### 6.2 如果想在命令面板增加一个预设按钮

文件：

- `zrcs_gui/modules/command/CommandPanel.cpp`

在合适的分组里加一行：

```cpp
addCommandRow(layout, "MyCommand", {"轴号", "值"}, {0, 0});
```

如果是无参数命令，也可以加：

```cpp
addButtonRow(layout, {"MyCommand"});
```

### 6.3 如果想在主界面其他按钮触发

例如首页快捷按钮、Jog 面板、IO 面板触发某个命令，则需要改：

- `zrcs_gui/core/MainWindow.cpp`

一般最终都会调用：

```cpp
sendMotionCommand("MyCommand", {0.0, 123.4});
```

### 6.4 GUI 到 NRT 的实际发送位置

真正发包的位置在：

- `zrcs_gui/communication/ZmqClient.cpp`

当前普通命令走的是：

- `ZMQClientWorker::sendCommand()`
- protobuf 消息类型：`zrcs_message::MotionCommand`

因此，普通命令不需要额外修改 `message.proto`

前提是你接受当前协议格式：

- `string command`
- `repeated double args`

## 7. 第六步：什么时候需要修改 `message.proto`

文件：

- `zrcs_common/message/message.proto`

普通命令新增时，通常不需要改这个文件，因为系统已经有：

```proto
message MotionCommand {
    string command = 1;
    repeated double args = 2;
}
```

只有以下情况才需要改 `message.proto`：

- 你要引入新的强类型 protobuf 命令
- 你要扩展状态发布内容
- 你要新增上位机订阅的数据结构

例如：

- 新增 `XXXStatus`
- 新增 `TypedCommand` 的 `oneof payload`
- 新增 `SystemStatus` 字段

改完后需要重新构建，让 protobuf 代码重新生成。

## 8. 第七步：如何验证命令新增是否成功

推荐按下面顺序检查。

### 8.1 编译检查

重新构建：

```bash
cmake --build build --target zrcsrt zrcsnrt zrcsgui --config Debug
```

### 8.2 GUI 手工发送

在上位机命令面板中发送：

```text
MyCommand(0, 123.4)
```

### 8.3 查看 NRT 日志

确认 NRT 侧是否打印了类似：

```text
[ZMQServer] MotionCommand: cmd='MyCommand'
[RtBridge] Sent 'MyCommand' seq=...
```

如果这里报：

```text
Unknown command 'MyCommand', not registered in cmdNameToId
```

说明你漏改了：

- `zrcs_rt/system/CmdIds.h`

### 8.4 查看 RT 日志

确认 RT 侧是否打印了类似：

```text
调度命令: MyCommand(seq=...)
```

如果 RT 报：

```text
未注册的命令: MyCommand(seq=...), 已忽略
```

说明你大概率漏改了：

- `zrcs_rt/command/CmdHead.h`

或者漏写了：

- `REGISTERCMD(MyCommand);`

### 8.5 查看命令执行结果

RT 执行后会更新共享内存中的：

- `lastCmdSeq`
- `lastCmdResult`

可通过 `RtBridge::waitForCompletion()` 或日志确认成功失败。

## 9. 推荐新增命令检查清单

新增一个普通 RT 命令时，建议逐项核对：

- 已新增 `zrcs_rt/command/MyCommand.h`
- 已新增 `zrcs_rt/command/MyCommand.cpp`
- 头文件中已写 `CMD_DEFINE(...)`
- 源文件底部已写 `REGISTERCMD(MyCommand);`
- `nodeName_` 与命令名一致
- `zrcs_rt/system/CmdIds.h` 已加入枚举项
- `zrcs_rt/system/CmdIds.h` 已加入 `cmdIdToName()` 表项
- `zrcs_rt/system/CmdIds.h` 已加入 `cmdNameToId()` 映射
- `zrcs_rt/system/CmdIds.h` 已更新 `SENTINEL`
- `zrcs_rt/command/CmdHead.h` 已加入头文件
- 如果 GUI 需要预设按钮，已修改 `zrcs_gui/modules/command/CommandPanel.cpp`
- 如果是系统控制类命令，已修改 `zrcs_nrt/zmq_server/ZmqServer.h`
- 如果需要新的共享内存桥接能力，已修改 `zrcs_nrt/rt_bridge/RtBridge.h`
- 已重新编译并做实际发送验证

## 10. 一个最常见的问题

### 问题：GUI 能发出命令，但 RT 提示“未注册的命令”

优先检查：

1. 是否在 `CmdIds.h` 中加入了该命令名和 ID
2. 是否在 `CmdHead.h` 中加入了该命令头文件
3. `.cpp` 底部是否写了 `REGISTERCMD(YourCommand);`
4. 命令名字符串是否完全一致，包括大小写

### 问题：NRT 提示 `Unknown command`

优先检查：

1. `cmdNameToId()` 是否加入了映射
2. GUI 发送的字符串是否拼写正确

### 问题：RT 收到命令但参数不对

优先检查：

1. `CMD_DEFINE(... PARAM(...))` 的参数顺序
2. GUI 发送参数顺序
3. `init()` 中读取 `command_->args[...]` 的下标是否正确

## 11. 参考文件

建议优先参考以下现成实现：

- RT 命令示例：`zrcs_rt/command/SetZero.h`
- RT 命令实现：`zrcs_rt/command/SetZero.cpp`
- 命令注册宏：`zrcs_rt/system/CmdMeta.h`
- 命令 ID 表：`zrcs_rt/system/CmdIds.h`
- 命令头汇总：`zrcs_rt/command/CmdHead.h`
- RT 调度器：`zrcs_rt/system/NodeManager.cpp`
- NRT 桥接：`zrcs_nrt/rt_bridge/RtBridge.h`
- NRT ZMQ 服务端：`zrcs_nrt/zmq_server/ZmqServer.h`
- GUI 命令面板：`zrcs_gui/modules/command/CommandPanel.cpp`
- GUI ZMQ 客户端：`zrcs_gui/communication/ZmqClient.cpp`

## 12. 补充说明：关于 `tool/gen_cmd_registry.py`

仓库中仍存在：

- `tool/gen_cmd_registry.py`

它代表的是旧的自动生成方案思路，但当前实际代码路径已经切到手工维护：

- `CmdIds.h`
- `CmdHead.h`

因此，当前新增命令时应以手工维护这两处为准，不要只改生成脚本或只等生成文件变化。

# ZMQ 通信集成指南

## 架构概览

```
+----------------------------------------------------------+
|                 上位机/远程客户端                          |
|              (Python/C++/其他语言)                        |
+----------------------------+-----------------------------+
                             | ZMQ + Protobuf (TCP:5555)
                             v
+----------------------------------------------------------+
|                   ZRCS NRT 进程                           |
|  +------------------------------------------------------+|
|  |  ZMQ 服务器 (zmqServer.h)                            ||
|  |  - 监听 TCP:5555                                     ||
|  |  - 接收 Protobuf 消息                                ||
|  |  - 反序列化为 Command 结构                           ||
|  +-------------------------+----------------------------+||
|                            |                              |
|  +-------------------------v----------------------------+|
|  |  共享内存 (Boost IPC)                                ||
|  |  - commandQueue (NRT -> RT)                          ||
|  |  - statusQueue (RT -> NRT)                           ||
|  +-------------------------+----------------------------+|
+----------------------------+-----------------------------+
                             | Boost IPC
                             v
+----------------------------------------------------------+
|                   ZRCS RT 进程                            |
|  - 实时控制循环                                           |
|  - 从共享内存读取命令                                     |
|  - 执行运动控制                                           |
|  - 写入状态到共享内存                                     |
+----------------------------------------------------------+
```

## NRT 侧: ZMQ 服务器

### zmqServer.h

- 监听 TCP:5555（REP 模式）
- 接收 Protobuf 序列化的 MotionCommand 消息
- 反序列化为共享内存的 Command 结构
- 推送到 commandQueue
- 发送 ACK 回复

### 相关文件

| 文件 | 说明 |
|------|------|
| `zrcsNrt/zmqServer.h` | ZMQ 服务器实现 |
| `zrcsNrt/zmqClient.h` | 测试客户端 |
| `zrcsNrt/zmqClientTest.cpp` | 测试程序 |
| `zrcsNrt/main.cpp` | 主程序（启动 ZMQ 服务器） |

## GUI 侧: ZMQ 客户端

### zmqClient.h / zmqClient.cpp

- `ZMQClientWorker` -- 在后台线程运行的 ZMQ 客户端
- `ZMQClient` -- Qt 线程安全的主线程接口
- 便捷方法：`moveJ()`, `moveL()`, `stop()`, `enable()`, `disable()`, `reset()`, `home()` 等
- 信号槽通知连接状态

### 信号定义

```cpp
signals:
    void connected();                              // 连接成功
    void disconnected();                           // 连接断开
    void commandSent(const QString& cmd, bool ok); // 命令发送结果
    void errorOccurred(const QString& error);      // 错误发生
```

### 使用方式

方式 A -- 便捷方法:
```cpp
zmqClient->moveJ(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
zmqClient->moveL(100.0, 200.0, 300.0, 0.0, 0.0, 0.0);
zmqClient->stop();
zmqClient->enable();
```

方式 B -- 通用方法:
```cpp
zmqClient->sendCommand("CustomCommand", {arg1, arg2, arg3});
```

方式 C -- 统一接口（推荐）:
```cpp
sendMotionCommand("MoveJ", {0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
// 自动选择 ZMQ 或共享内存
```

## Protobuf 消息定义

文件: `zrcsCommon/message/message.proto`

```protobuf
message MotionCommand {
    string command = 1;       // 命令名称
    repeated double args = 2; // 参数列表
}

message AxisStatus {
    int32 axis_id = 1;
    double position = 2;
    double velocity = 3;
    double torque = 4;
}

message SystemStatus {
    repeated AxisStatus axes = 1;
    string system_state = 2;
}
```

## 通信流程

### 命令发送流程

```
上位机 (Python/C++)
  |-- 创建 MotionCommand
  |-- 序列化为字节流
  +-- 通过 ZMQ 发送到 NRT:5555

NRT 进程
  |-- ZMQ 服务器接收字节流
  |-- 反序列化为 MotionCommand
  |-- 转换为 Command 结构
  |-- 推送到 commandQueue
  +-- 发送 "OK" 回复

RT 进程
  |-- 从 commandQueue 读取命令
  |-- 解析命令和参数
  |-- 执行运动控制
  +-- 更新状态到 statusQueue
```

## 支持的命令

| 命令 | 参数 | 说明 |
|------|------|------|
| MoveJ | j1, j2, j3, j4, j5, j6 | 关节运动 |
| MoveL | x, y, z, rx, ry, rz | 直线运动 |
| MoveC | x1, y1, z1, x2, y2, z2 | 圆弧运动 |
| Stop | -- | 停止运动 |
| Enable | -- | 使能系统 |
| Disable | -- | 禁用系统 |
| Reset | -- | 复位系统 |
| Home | -- | 回零 |
| Jog | axis, direction, speed | 点动 |

## 客户端示例

### Python 客户端

```python
#!/usr/bin/env python3
import zmq
import sys
sys.path.insert(0, '/path/to/generated/protobuf')
from message_pb2 import MotionCommand

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

# 发送 MoveJ 命令
cmd = MotionCommand()
cmd.command = "MoveJ"
cmd.args.extend([0.0, 0.0, 0.0, 0.0, 0.0, 0.0])

socket.send(cmd.SerializeToString())
reply = socket.recv()
print(f"Reply: {reply.decode()}")
```

或使用封装好的客户端：

```python
from zrcs_client import ZRCSClient

client = ZRCSClient("localhost", 5555)
client.connect()
client.moveJ(0, 0, 0, 0, 0, 0)
client.moveL(100, 200, 300, 0, 0, 0)
client.stop()
client.enable()
client.disconnect()
```

### C++ 客户端

```cpp
#include "zmqClient.h"

ZMQClient client;
client.connect();
client.sendCommand("MoveJ", {0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
client.sendCommand("Stop");
client.disconnect();
```

### Qt GUI 中使用

```cpp
// GUI 自动集成，启动时自动连接 NRT 进程
// 手动发送命令：
zmqClient->moveJ(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
zmqClient->stop();
```

## 故障转移机制

GUI 实现了自动故障转移：优先使用 ZMQ，ZMQ 连接失败或断开时自动回退到共享内存，ZMQ 客户端持续尝试重新连接。

```cpp
void MainWindow::sendMotionCommand(const QString& command,
                                    const QVector<double>& args)
{
    if (useZMQ_ && zmqClient->isConnected()) {
        // 优先使用 ZMQ
        zmqClient->sendCommand(command, args);
    } else {
        // 回退到共享内存
        Command cmd;
        // ... 构建命令 ...
        nrtProcess->shared_block_->commandQueue.push(cmd);
    }
}
```

## 性能指标

| 指标 | 值 |
|------|-----|
| ZMQ 延迟 | < 1ms (本地) |
| 命令队列大小 | 16 条 |
| 最大参数数 | 10 个 double |
| 吞吐量 | > 1000 cmd/s |
| 超时时间 | 5000ms (可配置) |

## 编译依赖

### Ubuntu/Debian

```bash
sudo apt-get install libzmq3-dev libcppzmq-dev libprotobuf-dev protobuf-compiler
```

### macOS

```bash
brew install zmq cppzmq protobuf
```

### Windows (vcpkg)

```bash
vcpkg install zmq cppzmq protobuf
```

## 构建和运行

### 编译

```bash
cd zrcs-dev
mkdir build && cd build
cmake ..
make

# 或编译特定目标
make zrcsnrt    # NRT 进程
make zrcsgui    # GUI
make zrcsrt     # RT 进程
```

### 运行

终端 1 -- 启动 NRT 进程:
```bash
./bin/zrcsnrt
# 输出:
# ZRCS Non-Real-Time Process Started
# [NRT] SharedBlock initialized
# [ZMQServer] Initialized on tcp://*:5555
# [NRT] ZMQ server started, waiting for commands...
```

终端 2 -- 启动 GUI:
```bash
./bin/zrcsgui
# 状态栏显示: "已连接到 NRT 进程 (ZMQ)"
```

终端 3 -- 发送测试命令:
```bash
./bin/zmqClientTest
# 或
python3 ../tool/zrcs_client.py
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 连接被拒绝 | NRT 进程未启动 | 启动 `./bin/zrcsnrt` |
| 命令超时 | NRT 进程响应慢 | 增加超时时间 |
| Protobuf 编译失败 | 消息格式不匹配 | 重新编译 protobuf |
| GUI 显示"已断开连接" | 正常，会自动重连 | 等待或检查 NRT 进程 |
| CMake 找不到 cppzmq | 未安装 cppzmq | 安装 cppzmq 开发包 |

### 配置修改

修改 ZMQ 端口:
- NRT 侧: `zrcsNrt/zmqServer.h` 中的 `ENDPOINT`
- GUI 侧: `zrcsGui/mainwindow.cpp` 中的端口号

修改超时时间:
- `zrcsNrt/zmqServer.h` 中的 `RECV_TIMEOUT`

## 扩展建议

1. **状态反馈** -- 从 RT 进程读取状态并通过 ZMQ 发送回客户端
2. **命令优先级** -- 添加优先级队列
3. **远程监控** -- 支持远程连接和监控
4. **日志记录** -- 记录所有命令和状态变化
5. **性能分析** -- 统计命令延迟和吞吐量
6. **安全认证** -- 生产环境启用 CurveZMQ 加密认证

## 文件清单

```
zrcs-dev/
  zrcsNrt/
    main.cpp                 # 主程序（已集成 ZMQ 服务器）
    zmqServer.h              # ZMQ 服务器
    zmqClient.h              # 测试客户端
    zmqClientTest.cpp        # 测试程序
    CMakeLists.txt           # 构建配置
  zrcsGui/
    mainwindow.h             # GUI 主窗口（已集成 ZMQ）
    mainwindow.cpp           # GUI 实现
    zmqClient.h              # Qt ZMQ 客户端
    zmqClient.cpp            # 客户端实现
    CMakeLists.txt           # 构建配置
  zrcsCommon/
    message/
      message.proto          # Protobuf 消息定义
  tool/
    zrcs_client.py           # Python 测试客户端
```

---

版本: 2.0
最后更新: 2026-03-18

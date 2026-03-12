# ZRCS NRT (Non-Real-Time) 进程 - ZMQ 命令接收模块

## 架构概述

```
┌─────────────────┐
│   上位机/GUI    │
│  (Upper Host)   │
└────────┬────────┘
         │ ZMQ + Protobuf
         │ (TCP:5555)
         ▼
┌─────────────────────────────────────┐
│   ZRCS NRT 进程 (zrcsnrt)           │
│  ┌─────────────────────────────────┐│
│  │  ZMQ Server (zmqServer.h)       ││
│  │  - 接收 Protobuf 序列化命令     ││
│  │  - 反序列化为 Command 结构      ││
│  └──────────────┬──────────────────┘│
│                 │                    │
│  ┌──────────────▼──────────────────┐│
│  │  共享内存 (Shared Memory)       ││
│  │  - commandQueue (NRT -> RT)     ││
│  │  - statusQueue (RT -> NRT)      ││
│  └──────────────┬──────────────────┘│
└─────────────────┼────────────────────┘
                  │ Boost IPC
                  ▼
         ┌─────────────────┐
         │  ZRCS RT 进程   │
         │  (zrcsrt)       │
         │  实时控制循环   │
         └─────────────────┘
```

## 文件说明

### zmqServer.h
ZMQ 服务器实现，负责：
- 监听 TCP:5555 端口
- 接收 Protobuf 序列化的 MotionCommand 消息
- 反序列化并转换为共享内存 Command 结构
- 推送到 commandQueue
- 发送 ACK 回复给客户端

### zmqClient.h
ZMQ 客户端实现（用于测试和上位机集成），提供：
- `connect()` - 连接到 NRT 进程
- `sendCommand(command, args)` - 发送命令

### zmqClientTest.cpp
测试程序，演示如何发送各种命令

### main.cpp
NRT 进程主程序，负责：
- 初始化共享内存
- 启动 ZMQ 服务器
- 维持主循环

## 编译

确保已安装依赖：
```bash
# Ubuntu/Debian
sudo apt-get install libzmq3-dev libcppzmq-dev libprotobuf-dev protobuf-compiler

# macOS
brew install zmq cppzmq protobuf

# Windows (vcpkg)
vcpkg install zmq cppzmq protobuf
```

编译项目：
```bash
cd /path/to/zrcs-dev
mkdir build && cd build
cmake ..
make
```

## 使用方法

### 1. 启动 NRT 进程
```bash
./bin/zrcsnrt
```

输出示例：
```
ZRCS Non-Real-Time Process Started
[NRT] SharedBlock initialized
[ZMQServer] Initialized on tcp://*:5555
[NRT] ZMQ server started, waiting for commands...
```

### 2. 发送命令（客户端）

#### 方式 A：使用测试程序
```bash
./bin/zmqClientTest
```

#### 方式 B：在你的上位机代码中使用
```cpp
#include "zmqClient.h"

ZMQClient client;
client.connect();

// 发送 MoveJ 命令
client.sendCommand("MoveJ", {0.0, 0.0, 0.0, 0.0, 0.0, 0.0});

// 发送 MoveL 命令
client.sendCommand("MoveL", {100.0, 200.0, 300.0, 0.0, 0.0, 0.0});

// 发送 Stop 命令
client.sendCommand("Stop");
```

#### 方式 C：使用 Python 客户端
```python
import zmq
import sys
sys.path.insert(0, '/path/to/generated/protobuf')
from message_pb2 import MotionCommand

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

# 创建命令
cmd = MotionCommand()
cmd.command = "MoveJ"
cmd.args.extend([0.0, 0.0, 0.0, 0.0, 0.0, 0.0])

# 发送
socket.send(cmd.SerializeToString())
reply = socket.recv()
print(f"Reply: {reply.decode()}")
```

## 命令格式

### Protobuf 定义 (message.proto)
```protobuf
message MotionCommand {
    string command = 1;      // 命令名称
    repeated double args = 2; // 参数列表
}
```

### 支持的命令示例
- `MoveJ` - 关节运动 (6个关节角度)
- `MoveL` - 直线运动 (X, Y, Z, Rx, Ry, Rz)
- `MoveC` - 圆弧运动
- `Stop` - 停止运动
- `Enable` - 使能系统
- `Disable` - 禁用系统
- 其他自定义命令

## 共享内存结构

命令通过 `SharedBlock::commandQueue` 传递给 RT 进程：

```cpp
struct Command {
    char cmd[100];      // 命令名称
    double args[10];    // 最多10个参数
};

// 环形缓冲区 (SPSC - Single Producer Single Consumer)
SPSCRingBuffer<Command, 16> commandQueue;
```

## 实时进程集成

RT 进程从共享内存读取命令：

```cpp
Command cmd;
while (shared_block_->commandQueue.pop(cmd)) {
    // 处理命令
    processCommand(cmd);
}
```

## 性能特性

- **低延迟**：ZMQ 使用高效的 IPC 机制
- **无锁**：SPSC 环形缓冲区使用原子操作，无互斥锁
- **缓冲**：16 个命令的缓冲区，防止丢失
- **超时处理**：ZMQ 接收超时 1000ms，防止阻塞

## 故障排查

### 问题：连接被拒绝
```
[ZMQClient] Connection error: Connection refused
```
**解决**：确保 NRT 进程已启动

### 问题：命令队列满
```
[ZMQServer] Command queue full
```
**解决**：RT 进程处理命令太慢，检查 RT 进程状态

### 问题：Protobuf 解析失败
```
[ZMQServer] Failed to parse protobuf message
```
**解决**：确保客户端和服务器使用相同的 message.proto 定义

## 扩展建议

1. **添加命令优先级**：在 Command 结构中添加优先级字段
2. **命令超时**：添加时间戳和超时检测
3. **命令确认**：在 statusQueue 中返回命令执行结果
4. **日志记录**：记录所有接收和发送的命令
5. **性能监控**：统计命令延迟和吞吐量

# ZRCS ZMQ 集成 - 快速开始指南

## 5 分钟快速开始

### 第 1 步：编译

```bash
cd /path/to/zrcs-dev
mkdir build && cd build
cmake ..
make
```

### 第 2 步：启动 NRT 进程

```bash
./bin/zrcsnrt
```

预期输出：
```
ZRCS Non-Real-Time Process Started
[NRT] SharedBlock initialized
[ZMQServer] Initialized on tcp://*:5555
[NRT] ZMQ server started, waiting for commands...
```

### 第 3 步：启动 GUI

在另一个终端：
```bash
./bin/zrcsgui
```

GUI 状态栏应显示：`已连接到 NRT 进程 (ZMQ)`

### 第 4 步：发送测试命令

在第三个终端：
```bash
./bin/zmqClientTest
```

或使用 Python：
```bash
python3 ../tool/zrcs_client.py
```

## 核心概念

### 三层架构

```
上位机/客户端 (Python/C++/GUI)
        ↓ ZMQ + Protobuf
    NRT 进程 (zrcsnrt)
        ↓ 共享内存
    RT 进程 (zrcsrt)
```

### 通信方式

1. **ZMQ (推荐)**：用于上位机与 NRT 进程通信
2. **共享内存**：用于 NRT 与 RT 进程通信
3. **自动故障转移**：ZMQ 失败时自动使用共享内存

## 常用命令

### 在 Python 中

```python
from zrcs_client import ZRCSClient

client = ZRCSClient()
client.connect()

# 关节运动
client.move_j(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

# 直线运动
client.move_l(100.0, 200.0, 300.0, 0.0, 0.0, 0.0)

# 停止
client.stop()

# 使能
client.enable()

client.disconnect()
```

### 在 C++ GUI 中

```cpp
// 自动集成，无需额外代码
// GUI 会自动连接并发送命令

// 或手动发送
zmqClient->moveJ(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
zmqClient->stop();
```

### 在 C++ 中

```cpp
#include "zmqClient.h"

ZMQClient client;
client.connect();
client.sendCommand("MoveJ", {0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
client.disconnect();
```

## 文件位置

| 文件 | 位置 | 说明 |
|------|------|------|
| NRT 主程序 | `zrcsNrt/main.cpp` | 启动 ZMQ 服务器 |
| NRT ZMQ 服务器 | `zrcsNrt/zmqServer.h` | 接收命令 |
| GUI 客户端 | `zrcsGui/zmqClient.h/cpp` | 发送命令 |
| Python 客户端 | `tool/zrcs_client.py` | Python 测试工具 |
| Protobuf 定义 | `zrcsCommon/message/message.proto` | 消息格式 |

## 配置修改

### 修改 ZMQ 端口

在 `zrcsNrt/zmqServer.h` 中：
```cpp
static constexpr const char* ENDPOINT = "tcp://*:5555";  // 修改端口
```

在 `zrcsGui/mainwindow.cpp` 中：
```cpp
zmqClient = new ZMQClient("localhost", 5555, this);  // 修改端口
```

### 修改超时时间

在 `zrcsNrt/zmqServer.h` 中：
```cpp
static constexpr int RECV_TIMEOUT = 1000;  // 修改为需要的毫秒数
```

## 故障排查

| 问题 | 原因 | 解决方案 |
|------|------|--------|
| 连接被拒绝 | NRT 进程未启动 | 启动 `./bin/zrcsnrt` |
| 命令超时 | NRT 进程响应慢 | 增加超时时间 |
| Protobuf 错误 | 消息格式不匹配 | 重新编译 protobuf |
| GUI 显示"已断开连接" | 正常，会自动重连 | 等待或检查 NRT 进程 |

## 下一步

1. **查看详细文档**：
   - `zrcsNrt/README.md` - NRT 进程详细说明
   - `zrcsGui/ZMQ_INTEGRATION.md` - GUI 集成详细说明
   - `ZMQ_PROTOBUF_INTEGRATION.md` - 完整架构说明

2. **自定义命令**：
   - 修改 `message.proto` 添加新消息类型
   - 在 RT 进程中处理新命令

3. **性能优化**：
   - 调整缓冲区大小
   - 优化命令处理逻辑

4. **远程连接**：
   - 修改 NRT 进程监听地址为 `0.0.0.0`
   - 配置防火墙允许 5555 端口

## 支持的命令列表

```
MoveJ      - 关节运动 (6个关节角度)
MoveL      - 直线运动 (X, Y, Z, Rx, Ry, Rz)
MoveC      - 圆弧运动 (起点和终点)
Stop       - 停止运动
Enable     - 使能系统
Disable    - 禁用系统
Reset      - 复位系统
Home       - 回零
Jog        - 点动 (轴号, 方向, 速度)
```

## 性能指标

- **通信延迟**：< 1ms (本地)
- **命令队列**：16 条
- **最大参数**：10 个 double
- **吞吐量**：> 1000 cmd/s

## 常见问题

**Q: 如何在远程机器上运行客户端？**

A: 修改客户端连接地址：
```python
client = ZRCSClient(host="192.168.1.100", port=5555)
```

并修改 NRT 进程监听地址为 `0.0.0.0`。

**Q: 如何添加自定义命令？**

A: 
1. 修改 `message.proto` 添加新消息类型
2. 重新编译 protobuf
3. 在 RT 进程中处理新命令

**Q: ZMQ 连接失败时会怎样？**

A: GUI 会自动回退到共享内存模式，命令仍会被执行。

**Q: 如何监控命令执行状态？**

A: 查看 RT 进程的 `statusQueue`，或在 NRT 进程中添加状态反馈。

## 获取帮助

- 查看各模块的 README.md 文件
- 查看源代码中的注释
- 检查 Qt 输出窗口的调试信息
- 查看 ZMQ 服务器的日志输出

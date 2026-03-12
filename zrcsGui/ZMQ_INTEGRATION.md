# zrcsGui ZMQ 客户端集成指南

## 概述

zrcsGui 已集成 ZMQ 客户端，支持通过 ZMQ + Protobuf 与 NRT 进程通信，同时保持与共享内存的兼容性。

## 架构

```
┌─────────────────────────┐
│   zrcsGui (Qt GUI)      │
│  ┌───────────────────┐  │
│  │  ZMQClient        │  │
│  │  (后台线程)       │  │
│  └─────────┬─────────┘  │
│            │ ZMQ        │
└────────────┼────────────┘
             │ TCP:5555
             ▼
    ┌─────────────────┐
    │  zrcsNrt        │
    │  (NRT 进程)     │
    └────────┬────────┘
             │ 共享内存
             ▼
    ┌─────────────────┐
    │  zrcsRt         │
    │  (RT 进程)      │
    └─────────────────┘
```

## 文件说明

### zmqClient.h / zmqClient.cpp
- `ZMQClientWorker`: 在后台线程运行的 ZMQ 客户端
- `ZMQClient`: Qt 线程安全的客户端接口
- 提供便捷方法：`moveJ()`, `moveL()`, `stop()`, `enable()` 等

### mainwindow.h / mainwindow.cpp
- 集成 ZMQ 客户端
- 自动故障转移：ZMQ 连接失败时回退到共享内存
- 新增槽函数：
  - `onZMQConnected()`: 连接成功
  - `onZMQDisconnected()`: 连接断开
  - `onZMQCommandSent()`: 命令发送结果
  - `onZMQError()`: 错误处理
- 新增方法：`sendMotionCommand()` - 统一的命令发送接口

## 使用方法

### 1. 编译

```bash
cd /path/to/zrcs-dev
mkdir build && cd build
cmake ..
make
```

### 2. 运行

启动 NRT 进程：
```bash
./bin/zrcsnrt
```

启动 GUI：
```bash
./bin/zrcsgui
```

GUI 会自动尝试连接到 NRT 进程。连接状态显示在状态栏。

### 3. 在代码中使用

#### 方式 A：使用便捷方法

```cpp
// 在 mainwindow.cpp 中
zmqClient->moveJ(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
zmqClient->moveL(100.0, 200.0, 300.0, 0.0, 0.0, 0.0);
zmqClient->stop();
zmqClient->enable();
```

#### 方式 B：使用通用方法

```cpp
zmqClient->sendCommand("CustomCommand", {arg1, arg2, arg3});
```

#### 方式 C：使用统一接口（推荐）

```cpp
sendMotionCommand("MoveJ", {0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
```

这个方法会自动选择 ZMQ 或共享内存。

## 信号和槽

### ZMQClient 信号

```cpp
signals:
    void connected();                              // 连接成功
    void disconnected();                           // 连接断开
    void commandSent(const QString& cmd, bool ok); // 命令发送结果
    void errorOccurred(const QString& error);      // 错误发生
```

### 连接示例

```cpp
connect(zmqClient, &ZMQClient::connected, this, [this]() {
    qDebug() << "Connected to NRT process";
});

connect(zmqClient, &ZMQClient::errorOccurred, this, [this](const QString& error) {
    qDebug() << "Error:" << error;
});
```

## 故障转移机制

GUI 实现了自动故障转移：

1. **优先使用 ZMQ**：如果 ZMQ 连接成功，命令通过 ZMQ 发送
2. **回退到共享内存**：如果 ZMQ 连接失败或断开，自动使用共享内存
3. **自动重连**：ZMQ 客户端会持续尝试重新连接

```cpp
void MainWindow::sendMotionCommand(const QString& command, const QVector<double>& args)
{
    if (useZMQ_ && zmqClient->isConnected()) {
        // 使用 ZMQ
        zmqClient->sendCommand(command, args);
    } else {
        // 回退到共享内存
        Command cmd;
        // ... 构建命令 ...
        nrtProcess->shared_block_->commandQueue.push(cmd);
    }
}
```

## 性能特性

- **非阻塞**：ZMQ 客户端在后台线程运行，不阻塞 GUI
- **低延迟**：ZMQ 使用高效的 IPC 机制
- **可靠**：自动故障转移确保命令不丢失
- **线程安全**：使用 Qt 的信号槽机制进行线程间通信

## 配置

### 修改连接地址

在 `mainwindow.cpp` 中修改：

```cpp
zmqClient = new ZMQClient("localhost", 5555, this);  // 修改 host 和 port
```

### 修改超时时间

在 `zmqClient.h` 中修改 `ZMQClientWorker` 构造函数的超时参数。

## 调试

### 查看连接状态

```cpp
if (zmqClient->isConnected()) {
    qDebug() << "Connected via ZMQ";
} else {
    qDebug() << "Using shared memory fallback";
}
```

### 启用详细日志

在 `zmqClient.cpp` 中已有 `qDebug()` 输出，可在 Qt Creator 的输出窗口查看。

## 扩展建议

1. **添加连接管理 UI**：显示连接状态、自动重连选项
2. **命令队列可视化**：显示待发送命令数量
3. **性能监控**：统计命令延迟、吞吐量
4. **命令历史**：记录发送的所有命令
5. **远程连接**：支持连接到远程 NRT 进程

## 常见问题

### Q: GUI 启动时显示"已断开连接"

A: 这是正常的。NRT 进程可能还未启动。GUI 会自动尝试重新连接。

### Q: 命令发送失败

A: 检查：
1. NRT 进程是否运行
2. 防火墙是否阻止 5555 端口
3. 查看 Qt 输出窗口的错误信息

### Q: 如何切换到纯共享内存模式

A: 注释掉 `mainwindow.cpp` 中的 ZMQ 初始化代码：

```cpp
// zmqClient = new ZMQClient("localhost", 5555, this);
// ...
// zmqClient->connectToServer();
```

## 相关文件

- `zrcsNrt/zmqServer.h` - NRT 进程的 ZMQ 服务器
- `zrcsNrt/zmqClient.h` - NRT 进程的 ZMQ 客户端（用于测试）
- `zrcsCommon/message/message.proto` - Protobuf 消息定义

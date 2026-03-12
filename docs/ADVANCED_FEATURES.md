# 高级功能实现指南

## 📋 目录

1. [轨迹可视化模块](#轨迹可视化模块)
2. [G-code 编辑器](#g-code-编辑器)
3. [远程监控功能](#远程监控功能)
4. [插件化架构](#插件化架构)

---

## 轨迹可视化模块

### 概述

支持 2D 和 3D 轨迹可视化，适用于不同的应用场景：
- **2D 轨迹**: 平面切割、打标、激光雕刻等
- **3D 轨迹**: 机械臂、多轴 CNC、3D 打印等

### 2D 轨迹可视化 (QGraphicsView)

#### 优势
- 使用 BSP 树自动优化渲染
- 支持几百万个轨迹点
- 缩放和平移流畅
- 自动视口裁剪

#### 使用示例

```cpp
#include "trajectory/trajectoryVisualizer.h"

// 创建 2D 可视化器
TrajectoryVisualizer2D *visualizer = new TrajectoryVisualizer2D();

// 添加轨迹点
QVector<QPointF> points;
points << QPointF(0, 0) << QPointF(10, 10) << QPointF(20, 0);
visualizer->addTrajectoryPoints(points);

// 设置颜色
visualizer->setTrajectoryColor(Qt::green);

// 缩放到适应视图
visualizer->fitInView();
```

### 3D 轨迹可视化 (QOpenGLWidget)

#### 优势
- 原生 OpenGL 渲染
- 极高的帧率
- 支持复杂的 3D 场景
- 可与 OpenCASCADE 集成

#### 使用示例

```cpp
#include "trajectory/trajectoryVisualizer.h"
#include <glm/glm.hpp>

// 创建 3D 可视化器
TrajectoryVisualizer3D *visualizer = new TrajectoryVisualizer3D();

// 添加 3D 轨迹点
QVector<glm::vec3> points3D;
points3D << glm::vec3(0, 0, 0) 
         << glm::vec3(10, 10, 5) 
         << glm::vec3(20, 0, 10);
visualizer->addTrajectoryPoints(points3D);

// 设置颜色
visualizer->setTrajectoryColor(glm::vec3(0.0f, 1.0f, 0.0f));

// 交互
visualizer->rotateView(45.0f, 30.0f);
visualizer->zoomView(1.5f);
```

### 轨迹面板集成

```cpp
// 创建轨迹面板
TrajectoryPanel *panel = new TrajectoryPanel();

// 加载 G-code 文件
panel->loadGCodeFile("path/to/file.gcode");

// 切换视图
panel->switchTo2DView();  // 或 switchTo3DView()

// 连接信号
connect(panel, &TrajectoryPanel::trajectoryLoaded, this, [](int count) {
    qDebug() << "轨迹点数:" << count;
});
```

---

## G-code 编辑器

### 概述

完整的 G-code 编辑器，包含：
- 语法高亮
- 行号显示
- 代码验证
- 统计信息

### 核心组件

#### 1. 语法高亮器

```cpp
class GCodeSyntaxHighlighter : public QSyntaxHighlighter {
    // 识别 G 指令、M 指令、坐标值、注释等
    // 为不同的元素应用不同的颜色
};
```

#### 2. 行号区域

```cpp
class LineNumberArea : public QWidget {
    // 显示行号
    // 与编辑器滚动同步
};
```

#### 3. G-code 编辑器

```cpp
class GCodeEditor : public QPlainTextEdit {
    // 集成语法高亮和行号
    // 提供验证和统计功能
};
```

### 使用示例

```cpp
#include "gcode/gcodeEditor.h"

// 创建编辑器
GCodeEditor *editor = new GCodeEditor();

// 打开文件
editor->openFile("path/to/file.gcode");

// 验证 G-code
QStringList errors = editor->validateGCode();
if (!errors.isEmpty()) {
    qDebug() << "验证错误:" << errors;
}

// 获取统计信息
auto stats = editor->getStatistics();
qDebug() << "总行数:" << stats.totalLines;
qDebug() << "G 指令数:" << stats.gCommands;
qDebug() << "M 指令数:" << stats.mCommands;
qDebug() << "预计时间:" << stats.estimatedTime << "分钟";
qDebug() << "总距离:" << stats.totalDistance << "mm";

// 保存文件
editor->saveFile("path/to/output.gcode");
```

### G-code 面板

```cpp
// 创建 G-code 面板
GCodePanel *panel = new GCodePanel();

// 打开文件
panel->openFile("path/to/file.gcode");

// 连接信号
connect(panel, &GCodePanel::contentChanged, this, [](const QString &content) {
    qDebug() << "内容已改变";
});

connect(panel, &GCodePanel::validationError, this, [](const QString &error) {
    qDebug() << "验证错误:" << error;
});
```

---

## 远程监控功能

### 概述

支持远程数据监控和视频流接收：
- ZMQ 数据接收（后台线程）
- RTSP 视频流
- OpenCV 视频处理
- 实时数据显示

### 架构

```
远程系统
    ↓ (ZMQ + Protobuf)
ZMQDataReceiver (后台线程)
    ↓ (Qt 信号槽)
RemoteDataMonitor (GUI 线程)
    ↓
界面更新
```

### 使用示例

#### 1. 数据监控

```cpp
#include "remote/remoteMonitor.h"

// 创建数据监控器
RemoteDataMonitor *monitor = new RemoteDataMonitor();

// 连接到远程服务器
monitor->connectToServer("tcp://192.168.1.100:5555");

// 连接信号
connect(monitor, &RemoteDataMonitor::dataUpdated, this, [](const QMap<QString, QVariant> &data) {
    qDebug() << "数据更新:" << data;
});

connect(monitor, &RemoteDataMonitor::connectionStatusChanged, this, [](bool connected) {
    qDebug() << "连接状态:" << (connected ? "已连接" : "已断开");
});
```

#### 2. 视频监控

```cpp
// 创建视频接收器
VideoStreamReceiver *videoReceiver = new VideoStreamReceiver();

// 连接到 RTSP 流
videoReceiver->connectToStream("rtsp://192.168.1.100:554/stream");

// 或连接到本地摄像头
// videoReceiver->connectToStream("0");  // 摄像头索引

// 连接信号
connect(videoReceiver, &VideoStreamReceiver::frameReceived, this, [](const QPixmap &frame) {
    // 处理视频帧
});
```

#### 3. 完整的远程监控面板

```cpp
// 创建远程监控面板
RemoteMonitorPanel *panel = new RemoteMonitorPanel();

// 连接到远程系统
panel->connectToRemoteSystem(
    "tcp://192.168.1.100:5555",  // ZMQ 端点
    "rtsp://192.168.1.100:554/stream"  // 视频 URL
);

// 连接信号
connect(panel, &RemoteMonitorPanel::connectionStatusChanged, this, [](bool connected) {
    qDebug() << "远程连接状态:" << (connected ? "已连接" : "已断开");
});
```

### 线程安全

ZMQ 数据接收在后台线程运行，通过 Qt 信号槽机制安全地传递数据到 GUI 线程：

```cpp
// 后台线程
void ZMQDataReceiver::receiveData() {
    while (running) {
        QByteArray data = receiveFromZMQ();
        // 通过信号传递到 GUI 线程
        emit dataReceived(data);
    }
}

// GUI 线程
void RemoteDataMonitor::onDataReceived(const QByteArray &data) {
    // 安全地更新 UI
    updateDataDisplay(parseData(data));
}
```

---

## 插件化架构

### 概述

使用 Qt 的插件机制实现可扩展的架构：
- 定义接口
- 编写插件
- 动态加载
- 运行时管理

### 核心概念

#### 1. 定义接口

```cpp
class IPlugin {
public:
    virtual QString getName() const = 0;
    virtual QString getVersion() const = 0;
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
    virtual QWidget *getWidget() = 0;
};

Q_DECLARE_INTERFACE(IPlugin, "com.zrcs.IPlugin/1.0")
```

#### 2. 实现插件

```cpp
// myPlugin.h
class MyPlugin : public QObject, public IPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.zrcs.IPlugin/1.0")
    Q_INTERFACES(IPlugin)

public:
    QString getName() const override { return "My Plugin"; }
    QString getVersion() const override { return "1.0"; }
    bool initialize() override { return true; }
    void cleanup() override {}
    QWidget *getWidget() override { return new QWidget(); }
};
```

#### 3. 加载插件

```cpp
#include "plugin/pluginManager.h"

// 创建插件管理器
PluginManager *manager = new PluginManager();

// 设置插件目录
manager->setPluginDirectory("./plugins");

// 加载所有插件
int count = manager->loadAllPlugins();
qDebug() << "加载了" << count << "个插件";

// 获取已加载的插件
QStringList plugins = manager->getLoadedPlugins();
for (const QString &name : plugins) {
    PluginInfo info = manager->getPluginInfo(name);
    qDebug() << "插件:" << info.name << "版本:" << info.version;
}

// 获取特定类型的插件
QVector<IToolPlugin *> toolPlugins = manager->getToolPlugins();
for (IToolPlugin *plugin : toolPlugins) {
    plugin->execute(QMap<QString, QVariant>());
}
```

#### 4. 插件容器

```cpp
// 创建插件容器
PluginContainer *container = new PluginContainer(manager);

// 添加插件到容器
container->addPlugin("My Plugin");

// 显示插件
container->showPlugin("My Plugin");

// 连接信号
connect(container, &PluginContainer::pluginActivated, this, [](const QString &name) {
    qDebug() << "插件激活:" << name;
});
```

### 编译插件

在 CMakeLists.txt 中：

```cmake
# 插件项目
add_library(myPlugin SHARED myPlugin.cpp)
target_link_libraries(myPlugin Qt5::Core Qt5::Widgets)

# 设置输出目录
set_target_properties(myPlugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins
)
```

### 插件开发最佳实践

1. **使用接口隔离** - 通过接口定义插件功能
2. **版本管理** - 在元数据中指定版本
3. **错误处理** - 插件加载失败时优雅降级
4. **资源管理** - 正确实现 initialize() 和 cleanup()
5. **线程安全** - 如果插件使用线程，确保线程安全

---

## 集成到主应用

### 更新 CMakeLists.txt

```cmake
# 添加子目录
add_subdirectory(trajectory)
add_subdirectory(gcode)
add_subdirectory(remote)
add_subdirectory(plugin)

# 链接库
target_link_libraries(${PROJECT_NAME}
    trajectoryLib
    gcodeLib
    remoteLib
    pluginLib
    Qt5::OpenGL
    Qt5::Multimedia
    OpenCV::Core
    OpenCV::VideoIO
)
```

### 在主窗口中集成

```cpp
#include "trajectory/trajectoryVisualizer.h"
#include "gcode/gcodeEditor.h"
#include "remote/remoteMonitor.h"
#include "plugin/pluginManager.h"

class MainWindowRefactored : public QMainWindow {
private:
    TrajectoryPanel *trajectoryPanel;
    GCodePanel *gcodePanel;
    RemoteMonitorPanel *remotePanel;
    PluginManager *pluginManager;
    
    void setupAdvancedFeatures() {
        // 创建轨迹可视化
        trajectoryPanel = new TrajectoryPanel();
        tabWidget->addTab(trajectoryPanel, "轨迹可视化");
        
        // 创建 G-code 编辑器
        gcodePanel = new GCodePanel();
        tabWidget->addTab(gcodePanel, "G-code 编辑");
        
        // 创建远程监控
        remotePanel = new RemoteMonitorPanel();
        tabWidget->addTab(remotePanel, "远程监控");
        
        // 初始化插件管理器
        pluginManager = new PluginManager();
        pluginManager->setPluginDirectory("./plugins");
        pluginManager->loadAllPlugins();
    }
};
```

---

## 性能优化建议

### 轨迹可视化
- 使用 LOD (Level of Detail) 技术
- 对大量点进行采样
- 使用 VBO (Vertex Buffer Object) 缓存

### G-code 编辑器
- 使用增量语法高亮
- 异步验证
- 缓存解析结果

### 远程监控
- 使用后台线程接收数据
- 限制更新频率
- 压缩视频流

### 插件系统
- 延迟加载插件
- 使用插件缓存
- 限制插件数量

---

## 参考资源

- [Qt Graphics View Framework](https://doc.qt.io/qt-5/graphicsview.html)
- [Qt OpenGL](https://doc.qt.io/qt-5/qtopengl-index.html)
- [Qt Plugin System](https://doc.qt.io/qt-5/plugins-howto.html)
- [ZeroMQ Guide](https://zguide.zeromq.org/)
- [OpenCV Documentation](https://docs.opencv.org/)

---

**版本**: 2.0  
**最后更新**: 2026-03-12

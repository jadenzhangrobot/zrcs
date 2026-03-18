# GUI 文件组织说明

## 推荐目录结构

```
zrcsGui/
  src/
    core/
      mainwindow_refactored.h          # 主窗口头文件
      mainwindow_refactored.cpp        # 主窗口实现
      main_refactored.cpp              # 应用入口
    components/
      statusIndicator.cpp              # 状态指示灯和坐标显示
      jogAndIOPanel.cpp                # 点动控制和 I/O 面板
      alarmPanel.cpp                   # 报警和诊断面板
    communication/
      zmqClient.h                      # ZMQ 客户端接口
      zmqClient.cpp                    # ZMQ 客户端实现
    config/
      zrcsStyles.h                     # 全局样式定义
      zrcsConfig.h                     # 配置管理系统

  modules/
    trajectory/
      trajectoryVisualizer.h           # 2D/3D 轨迹可视化
      trajectoryVisualizer.cpp         # 实现文件（待创建）
    gcode/
      gcodeEditor.h                    # G-code 编辑器
      gcodeEditor.cpp                  # 实现文件（待创建）
    remote/
      remoteMonitor.h                  # 远程监控
      remoteMonitor.cpp                # 实现文件（待创建）
    plugin/
      pluginInterface.h               # 插件接口定义
      pluginManager.h                  # 插件管理器
      pluginManager.cpp                # 实现文件（待创建）

  resources/
    ui/
      zrcsgui.ui                       # 原有 UI 文件
      mainwindow_refactored.ui         # 新 UI 文件
    style/
      dark_theme.qss                   # 深色主题样式表
      styleLoader.h                    # 样式加载器

  CMakeLists.txt                       # GUI 子项目构建配置
```

## 目录职责

### src/core/

主窗口和应用入口，是 GUI 应用的骨架。

| 文件 | 职责 |
|------|------|
| `mainwindow_refactored.h` | 定义 MainWindowRefactored 类及所有 UI 组件类 |
| `mainwindow_refactored.cpp` | 主窗口实现：界面构建、样式应用、信号槽连接、数据更新 |
| `main_refactored.cpp` | 应用入口：启动 NRT 进程、创建主窗口 |

### src/components/

独立的 UI 组件，每个文件实现一个或多个相关组件。

| 文件 | 包含的类 |
|------|----------|
| `statusIndicator.cpp` | StatusIndicator（状态指示灯）、AxisPositionDisplay（坐标显示） |
| `jogAndIOPanel.cpp` | JogControlPanel（点动控制）、IOPanel（I/O 面板） |
| `alarmPanel.cpp` | AlarmPanel（报警面板） |

### src/communication/

通信相关代码，负责与 NRT 进程的数据交换。

| 文件 | 职责 |
|------|------|
| `zmqClient.h` | ZMQ 客户端接口定义（ZMQClientWorker + ZMQClient） |
| `zmqClient.cpp` | ZMQ 客户端实现（后台线程、自动重连、故障转移） |

### src/config/

全局配置和样式定义。

| 文件 | 职责 |
|------|------|
| `zrcsStyles.h` | 颜色定义、样式表生成函数、按钮/标签样式 |
| `zrcsConfig.h` | 配置结构体（UI、通信、运动、安全、显示、日志） |

### modules/

高级功能模块，每个子目录为一个独立模块。

| 目录 | 功能 | 状态 |
|------|------|------|
| `trajectory/` | 2D/3D 轨迹可视化 | 框架已完成，待实现 |
| `gcode/` | G-code 编辑器 | 框架已完成，待实现 |
| `remote/` | 远程数据和视频监控 | 框架已完成，待实现 |
| `plugin/` | 插件化架构 | 框架已完成，待实现 |

### resources/

UI 文件和样式资源。

| 目录 | 内容 |
|------|------|
| `ui/` | Qt Designer UI 文件 |
| `style/` | QSS 样式表和样式加载器 |

## 文件迁移计划

当前 GUI 文件全部位于 `zrcsGui/` 根目录，建议按以下步骤迁移：

### 第一步：创建目录结构

```bash
cd zrcsGui
mkdir -p src/core src/components src/communication src/config
mkdir -p modules/trajectory modules/gcode modules/remote modules/plugin
mkdir -p resources/ui resources/style
```

### 第二步：移动文件

```bash
# 核心文件
mv mainwindow_refactored.h mainwindow_refactored.cpp main_refactored.cpp src/core/

# 组件文件
mv statusIndicator.cpp jogAndIOPanel.cpp alarmPanel.cpp src/components/

# 通信文件
mv zmqClient.h zmqClient.cpp src/communication/

# 配置文件
mv zrcsStyles.h zrcsConfig.h src/config/

# UI 文件
mv ui/*.ui resources/ui/

# 样式文件
mv style/*.qss style/*.h resources/style/

# 高级模块
mv trajectory/* modules/trajectory/
mv gcode/* modules/gcode/
mv remote/* modules/remote/
mv plugin/* modules/plugin/
```

### 第三步：更新 CMakeLists.txt

迁移后需更新 `zrcsGui/CMakeLists.txt` 中的源文件路径：

```cmake
set(SOURCES
    src/core/main_refactored.cpp
    src/core/mainwindow_refactored.cpp
    src/components/statusIndicator.cpp
    src/components/jogAndIOPanel.cpp
    src/components/alarmPanel.cpp
    src/communication/zmqClient.cpp
)

set(HEADERS
    src/core/mainwindow_refactored.h
    src/communication/zmqClient.h
    src/config/zrcsStyles.h
    src/config/zrcsConfig.h
)

set(UI_FILES
    resources/ui/zrcsgui.ui
    resources/ui/mainwindow_refactored.ui
)

# 包含路径
target_include_directories(${PROJECT_NAME} PRIVATE
    src/core
    src/components
    src/communication
    src/config
    modules
    resources
)

# 高级模块（按需启用）
# add_subdirectory(modules/trajectory)
# add_subdirectory(modules/gcode)
# add_subdirectory(modules/remote)
# add_subdirectory(modules/plugin)
```

## 组织规范的优势

### 1. 清晰的职责划分

每个目录有明确的职责范围，新开发者能快速定位代码位置。

### 2. 独立的模块开发

高级功能模块位于 `modules/` 目录，可以独立开发、测试和启用，不影响核心功能。

### 3. 便于团队协作

不同开发者可以在不同目录工作，减少代码冲突。

### 4. 支持渐进式构建

通过 CMake 的 `add_subdirectory` 和条件编译，可以按需构建模块，加快开发阶段的编译速度。

### 5. 资源集中管理

UI 文件和样式文件集中在 `resources/` 目录，便于非程序员（UI 设计师）修改界面和样式。

---

版本: 2.0
最后更新: 2026-03-18

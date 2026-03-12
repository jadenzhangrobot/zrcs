# 📁 ZRCS GUI 文件夹结构规划

## 推荐的文件夹组织方案

```
zrcsGui/
│
├── src/                          # 源代码目录
│   ├── core/                     # 核心窗口和主程序
│   │   ├── mainwindow_refactored.h
│   │   ├── mainwindow_refactored.cpp
│   │   └── main_refactored.cpp
│   │
│   ├── components/               # UI 组件
│   │   ├── statusIndicator.cpp
│   │   ├── jogAndIOPanel.cpp
│   │   └── alarmPanel.cpp
│   │
│   ├── communication/            # 通信相关
│   │   ├── zmqClient.h
│   │   └── zmqClient.cpp
│   │
│   └── config/                   # 配置文件
│       ├── zrcsConfig.h
│       └── zrcsStyles.h
│
├── include/                      # 头文件（可选，如果需要分离）
│   ├── core/
│   ├── components/
│   ├── communication/
│   └── config/
│
├── resources/                    # 资源文件
│   ├── ui/                       # UI 文件
│   │   └── mainwindow_refactored.ui
│   │
│   ├── style/                    # 样式文件
│   │   ├── dark_theme.qss
│   │   └── styleLoader.h
│   │
│   └── docs/                     # 文档
│       ├── QUICK_START.md
│       ├── GUI_REFACTOR.md
│       ├── ARCHITECTURE.md
│       ├── ADVANCED_FEATURES.md
│       └── UI_STYLE_GUIDE.md
│
├── modules/                      # 高级功能模块
│   ├── trajectory/               # 轨迹可视化
│   │   ├── trajectoryVisualizer.h
│   │   └── trajectoryVisualizer.cpp
│   │
│   ├── gcode/                    # G-code 编辑器
│   │   ├── gcodeEditor.h
│   │   └── gcodeEditor.cpp
│   │
│   ├── remote/                   # 远程监控
│   │   ├── remoteMonitor.h
│   │   └── remoteMonitor.cpp
│   │
│   └── plugin/                   # 插件系统
│       ├── pluginInterface.h
│       ├── pluginManager.h
│       └── pluginManager.cpp
│
├── CMakeLists.txt                # CMake 配置
├── INDEX.md                      # 文件索引
└── [其他文档文件]
```

## 文件分类说明

### src/core/ - 核心窗口和主程序
**包含文件**:
- `mainwindow_refactored.h` - 主窗口头文件
- `mainwindow_refactored.cpp` - 主窗口实现
- `main_refactored.cpp` - 应用入口

**用途**: 应用程序的核心，包含主窗口和启动逻辑

### src/components/ - UI 组件
**包含文件**:
- `statusIndicator.cpp` - 状态指示灯和坐标显示
- `jogAndIOPanel.cpp` - 点动控制和 I/O 面板
- `alarmPanel.cpp` - 报警和诊断面板

**用途**: 可复用的 UI 组件

### src/communication/ - 通信相关
**包含文件**:
- `zmqClient.h` - ZMQ 客户端头文件
- `zmqClient.cpp` - ZMQ 客户端实现

**用途**: 与后端通信的模块

### src/config/ - 配置文件
**包含文件**:
- `zrcsConfig.h` - 配置管理系统
- `zrcsStyles.h` - 全局样式定义

**用途**: 配置和样式管理

### resources/ui/ - UI 文件
**包含文件**:
- `mainwindow_refactored.ui` - Qt Designer UI 文件

**用途**: Qt Designer 可视化编辑的 UI 文件

### resources/style/ - 样式文件
**包含文件**:
- `dark_theme.qss` - 深色主题样式表
- `styleLoader.h` - 样式加载器

**用途**: 集中管理应用样式

### resources/docs/ - 文档
**包含文件**:
- `QUICK_START.md` - 快速开始指南
- `GUI_REFACTOR.md` - 详细重构指南
- `ARCHITECTURE.md` - 系统架构文档
- `ADVANCED_FEATURES.md` - 高级功能指南
- `UI_STYLE_GUIDE.md` - UI 和样式指南

**用途**: 项目文档

### modules/trajectory/ - 轨迹可视化
**包含文件**:
- `trajectoryVisualizer.h` - 轨迹可视化头文件
- `trajectoryVisualizer.cpp` - 轨迹可视化实现

**用途**: 2D/3D 轨迹可视化功能

### modules/gcode/ - G-code 编辑器
**包含文件**:
- `gcodeEditor.h` - G-code 编辑器头文件
- `gcodeEditor.cpp` - G-code 编辑器实现

**用途**: G-code 编辑和验证功能

### modules/remote/ - 远程监控
**包含文件**:
- `remoteMonitor.h` - 远程监控头文件
- `remoteMonitor.cpp` - 远程监控实现

**用途**: 远程数据和视频监控功能

### modules/plugin/ - 插件系统
**包含文件**:
- `pluginInterface.h` - 插件接口定义
- `pluginManager.h` - 插件管理器头文件
- `pluginManager.cpp` - 插件管理器实现

**用途**: 插件化架构支持

## 创建文件夹的步骤

### 方法 1: 使用 Windows 资源管理器
1. 打开 `c:\Users\64989\Desktop\zrcs-dev\zrcsGui`
2. 创建以下文件夹：
   - `src` → `core`, `components`, `communication`, `config`
   - `include` → `core`, `components`, `communication`, `config`
   - `resources` → `ui`, `style`, `docs`
   - `modules` → `trajectory`, `gcode`, `remote`, `plugin`

### 方法 2: 使用命令行
```bash
# 在 zrcsGui 目录下执行
mkdir src\core src\components src\communication src\config
mkdir include\core include\components include\communication include\config
mkdir resources\ui resources\style resources\docs
mkdir modules\trajectory modules\gcode modules\remote modules\plugin
```

### 方法 3: 使用 PowerShell
```powershell
$folders = @(
    'src/core', 'src/components', 'src/communication', 'src/config',
    'include/core', 'include/components', 'include/communication', 'include/config',
    'resources/ui', 'resources/style', 'resources/docs',
    'modules/trajectory', 'modules/gcode', 'modules/remote', 'modules/plugin'
)

foreach ($folder in $folders) {
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
}
```

## 文件迁移计划

### 第 1 步: 核心文件
```
mainwindow_refactored.h      → src/core/
mainwindow_refactored.cpp    → src/core/
main_refactored.cpp          → src/core/
```

### 第 2 步: 组件文件
```
statusIndicator.cpp          → src/components/
jogAndIOPanel.cpp            → src/components/
alarmPanel.cpp               → src/components/
```

### 第 3 步: 通信文件
```
zmqClient.h                  → src/communication/
zmqClient.cpp                → src/communication/
```

### 第 4 步: 配置文件
```
zrcsConfig.h                 → src/config/
zrcsStyles.h                 → src/config/
```

### 第 5 步: 资源文件
```
mainwindow_refactored.ui     → resources/ui/
dark_theme.qss               → resources/style/
styleLoader.h                → resources/style/
```

### 第 6 步: 文档文件
```
QUICK_START.md               → resources/docs/
GUI_REFACTOR.md              → resources/docs/
ARCHITECTURE.md              → resources/docs/
ADVANCED_FEATURES.md         → resources/docs/
UI_STYLE_GUIDE.md            → resources/docs/
```

### 第 7 步: 模块文件
```
trajectory/trajectoryVisualizer.h    → modules/trajectory/
gcode/gcodeEditor.h                  → modules/gcode/
remote/remoteMonitor.h               → modules/remote/
plugin/pluginInterface.h             → modules/plugin/
plugin/pluginManager.h               → modules/plugin/
```

## 更新 CMakeLists.txt

迁移文件后，需要更新 CMakeLists.txt 中的路径：

```cmake
# 源文件
set(SOURCES
    src/core/main_refactored.cpp
    src/core/mainwindow_refactored.cpp
    src/components/statusIndicator.cpp
    src/components/jogAndIOPanel.cpp
    src/components/alarmPanel.cpp
    src/communication/zmqClient.cpp
    ${GUI_PROTO_SRCS}
)

# 头文件
set(HEADERS
    src/core/mainwindow_refactored.h
    src/communication/zmqClient.h
    src/config/zrcsStyles.h
    src/config/zrcsConfig.h
    resources/style/styleLoader.h
)

# UI 文件
set(UI_FILES
    resources/ui/mainwindow_refactored.ui
)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcsGui/src
    ${CMAKE_SOURCE_DIR}/zrcsGui/include
    ${CMAKE_SOURCE_DIR}/zrcsGui/modules
)
```

## 优势

✅ **清晰的组织** - 文件按功能分类  
✅ **易于维护** - 相关文件集中在一起  
✅ **易于扩展** - 新功能可以添加新模块  
✅ **易于导航** - 快速找到需要的文件  
✅ **专业结构** - 符合大型项目的组织方式  

## 注意事项

1. **保持一致性** - 所有头文件和实现文件放在同一文件夹
2. **避免循环依赖** - 模块之间通过接口通信
3. **更新包含路径** - 迁移后更新所有 `#include` 语句
4. **更新 CMakeLists.txt** - 确保所有文件都被正确引用
5. **测试编译** - 迁移后重新编译确保没有问题

---

**版本**: 2.0  
**最后更新**: 2026-03-12

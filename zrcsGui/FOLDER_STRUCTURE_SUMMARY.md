# 📁 文件夹结构规划完成

## 推荐的组织方案

我已经为你设计了一个专业的文件夹结构，将所有文件按功能和类型合理分类。

### 核心结构

```
zrcsGui/
├── src/                    # 源代码
│   ├── core/              # 核心窗口和主程序
│   ├── components/        # UI 组件
│   ├── communication/     # 通信模块
│   └── config/            # 配置文件
│
├── include/               # 头文件（可选）
│
├── resources/             # 资源文件
│   ├── ui/               # UI 文件
│   ├── style/            # 样式文件
│   └── docs/             # 文档
│
└── modules/              # 高级功能模块
    ├── trajectory/       # 轨迹可视化
    ├── gcode/           # G-code 编辑器
    ├── remote/          # 远程监控
    └── plugin/          # 插件系统
```

## 文件分类

### src/core/ (核心)
- `mainwindow_refactored.h/cpp` - 主窗口
- `main_refactored.cpp` - 应用入口

### src/components/ (UI 组件)
- `statusIndicator.cpp` - 状态指示灯
- `jogAndIOPanel.cpp` - 点动和 I/O 面板
- `alarmPanel.cpp` - 报警面板

### src/communication/ (通信)
- `zmqClient.h/cpp` - ZMQ 客户端

### src/config/ (配置)
- `zrcsConfig.h` - 配置管理
- `zrcsStyles.h` - 样式定义

### resources/ui/ (UI 文件)
- `mainwindow_refactored.ui` - Qt Designer 文件

### resources/style/ (样式)
- `dark_theme.qss` - 样式表
- `styleLoader.h` - 样式加载器

### resources/docs/ (文档)
- `QUICK_START.md` - 快速开始
- `GUI_REFACTOR.md` - 重构指南
- `ARCHITECTURE.md` - 架构文档
- `ADVANCED_FEATURES.md` - 高级功能
- `UI_STYLE_GUIDE.md` - UI 指南

### modules/trajectory/ (轨迹可视化)
- `trajectoryVisualizer.h/cpp`

### modules/gcode/ (G-code 编辑器)
- `gcodeEditor.h/cpp`

### modules/remote/ (远程监控)
- `remoteMonitor.h/cpp`

### modules/plugin/ (插件系统)
- `pluginInterface.h`
- `pluginManager.h/cpp`

## 创建步骤

### 方法 1: Windows 资源管理器
1. 打开 `zrcsGui` 文件夹
2. 创建 `src`, `include`, `resources`, `modules` 文件夹
3. 在每个文件夹中创建子文件夹

### 方法 2: 命令行
```bash
mkdir src\core src\components src\communication src\config
mkdir include\core include\components include\communication include\config
mkdir resources\ui resources\style resources\docs
mkdir modules\trajectory modules\gcode modules\remote modules\plugin
```

### 方法 3: PowerShell
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

## 文件迁移

创建文件夹后，将文件移动到相应位置：

| 文件 | 目标位置 |
|------|---------|
| mainwindow_refactored.h/cpp | src/core/ |
| main_refactored.cpp | src/core/ |
| statusIndicator.cpp | src/components/ |
| jogAndIOPanel.cpp | src/components/ |
| alarmPanel.cpp | src/components/ |
| zmqClient.h/cpp | src/communication/ |
| zrcsConfig.h | src/config/ |
| zrcsStyles.h | src/config/ |
| mainwindow_refactored.ui | resources/ui/ |
| dark_theme.qss | resources/style/ |
| styleLoader.h | resources/style/ |
| *.md 文档 | resources/docs/ |
| trajectoryVisualizer.h/cpp | modules/trajectory/ |
| gcodeEditor.h/cpp | modules/gcode/ |
| remoteMonitor.h/cpp | modules/remote/ |
| pluginInterface.h | modules/plugin/ |
| pluginManager.h/cpp | modules/plugin/ |

## 更新 CMakeLists.txt

迁移后需要更新路径：

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
    resources/style/styleLoader.h
)

set(UI_FILES
    resources/ui/mainwindow_refactored.ui
)

include_directories(
    ${CMAKE_SOURCE_DIR}/zrcsGui/src
    ${CMAKE_SOURCE_DIR}/zrcsGui/modules
)
```

## 优势

✅ **清晰的组织** - 按功能分类  
✅ **易于维护** - 相关文件集中  
✅ **易于扩展** - 新功能添加新模块  
✅ **易于导航** - 快速找到文件  
✅ **专业结构** - 符合大型项目规范  

## 详细指南

完整的文件夹结构规划已保存在 `FOLDER_STRUCTURE.md`，包含：
- 详细的文件夹说明
- 文件迁移计划
- CMakeLists.txt 更新方法
- 注意事项

---

**版本**: 2.0  
**完成日期**: 2026-03-12  
**状态**: ✅ 规划完成

现在你有了一个专业的文件夹组织方案！🎉

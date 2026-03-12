# 📁 文件整理计划

## 当前文件列表分析

### 源代码文件 (需要整理)
- alarmPanel.cpp
- jogAndIOPanel.cpp
- main_refactored.cpp
- mainwindow_refactored.cpp
- mainwindow_refactored.h
- statusIndicator.cpp
- zmqClient.cpp
- zmqClient.h
- zrcsConfig.h
- zrcsStyles.h

### 文档文件 (需要整理)
- CODE_GENERATION_COMPLETE.md
- CODE_GENERATION_GUIDE.md
- FOLDER_STRUCTURE.md
- FOLDER_STRUCTURE_SUMMARY.md
- INDEX.md
- PROJECT_COMPLETION_SUMMARY.md
- QUICK_START.md
- SUMMARY.md

### 配置文件 (需要整理)
- CMakeLists.txt

## 推荐的整理方案

### 方案 A: 按功能分类 (推荐)

```
zrcsGui/
├── src/
│   ├── core/
│   │   ├── main_refactored.cpp
│   │   ├── mainwindow_refactored.h
│   │   └── mainwindow_refactored.cpp
│   │
│   ├── components/
│   │   ├── statusIndicator.cpp
│   │   ├── jogAndIOPanel.cpp
│   │   └── alarmPanel.cpp
│   │
│   ├── communication/
│   │   ├── zmqClient.h
│   │   └── zmqClient.cpp
│   │
│   └── config/
│       ├── zrcsConfig.h
│       └── zrcsStyles.h
│
├── modules/
│   ├── trajectory/
│   │   ├── trajectoryVisualizer.h
│   │   └── trajectoryVisualizer.cpp
│   │
│   ├── gcode/
│   │   ├── gcodeEditor.h
│   │   └── gcodeEditor.cpp
│   │
│   ├── remote/
│   │   ├── remoteMonitor.h
│   │   └── remoteMonitor.cpp
│   │
│   └── plugin/
│       ├── pluginInterface.h
│       ├── pluginManager.h
│       └── pluginManager.cpp
│
├── resources/
│   ├── ui/
│   │   └── mainwindow_refactored.ui
│   │
│   ├── style/
│   │   ├── dark_theme.qss
│   │   └── styleLoader.h
│   │
│   └── docs/
│       ├── QUICK_START.md
│       ├── GUI_REFACTOR.md
│       ├── ARCHITECTURE.md
│       ├── ADVANCED_FEATURES.md
│       ├── UI_STYLE_GUIDE.md
│       ├── CODE_GENERATION_GUIDE.md
│       ├── CODE_GENERATION_COMPLETE.md
│       ├── FOLDER_STRUCTURE.md
│       ├── PROJECT_COMPLETION_SUMMARY.md
│       └── SUMMARY.md
│
├── CMakeLists.txt
└── INDEX.md
```

### 方案 B: 简化方案

```
zrcsGui/
├── src/
│   ├── main_refactored.cpp
│   ├── mainwindow_refactored.h
│   ├── mainwindow_refactored.cpp
│   ├── statusIndicator.cpp
│   ├── jogAndIOPanel.cpp
│   ├── alarmPanel.cpp
│   ├── zmqClient.h
│   ├── zmqClient.cpp
│   ├── zrcsConfig.h
│   └── zrcsStyles.h
│
├── modules/
│   ├── trajectory/
│   ├── gcode/
│   ├── remote/
│   └── plugin/
│
├── resources/
│   ├── ui/
│   ├── style/
│   └── docs/
│
└── CMakeLists.txt
```

## 文件迁移清单

### 第 1 步: 创建文件夹结构
```bash
mkdir src\core src\components src\communication src\config
mkdir modules\trajectory modules\gcode modules\remote modules\plugin
mkdir resources\ui resources\style resources\docs
```

### 第 2 步: 迁移源代码文件

**src/core/**
- [ ] main_refactored.cpp
- [ ] mainwindow_refactored.h
- [ ] mainwindow_refactored.cpp

**src/components/**
- [ ] statusIndicator.cpp
- [ ] jogAndIOPanel.cpp
- [ ] alarmPanel.cpp

**src/communication/**
- [ ] zmqClient.h
- [ ] zmqClient.cpp

**src/config/**
- [ ] zrcsConfig.h
- [ ] zrcsStyles.h

### 第 3 步: 迁移资源文件

**resources/ui/**
- [ ] mainwindow_refactored.ui

**resources/style/**
- [ ] dark_theme.qss
- [ ] styleLoader.h

**resources/docs/**
- [ ] QUICK_START.md
- [ ] GUI_REFACTOR.md
- [ ] ARCHITECTURE.md
- [ ] ADVANCED_FEATURES.md
- [ ] UI_STYLE_GUIDE.md
- [ ] CODE_GENERATION_GUIDE.md
- [ ] CODE_GENERATION_COMPLETE.md
- [ ] FOLDER_STRUCTURE.md
- [ ] FOLDER_STRUCTURE_SUMMARY.md
- [ ] PROJECT_COMPLETION_SUMMARY.md
- [ ] SUMMARY.md

### 第 4 步: 更新 CMakeLists.txt

```cmake
# 源文件路径更新
set(SOURCES
    src/core/main_refactored.cpp
    src/core/mainwindow_refactored.cpp
    src/components/statusIndicator.cpp
    src/components/jogAndIOPanel.cpp
    src/components/alarmPanel.cpp
    src/communication/zmqClient.cpp
    modules/trajectory/trajectoryVisualizer.cpp
    modules/gcode/gcodeEditor.cpp
    modules/remote/remoteMonitor.cpp
    modules/plugin/pluginManager.cpp
)

# 头文件路径更新
set(HEADERS
    src/core/mainwindow_refactored.h
    src/communication/zmqClient.h
    src/config/zrcsStyles.h
    src/config/zrcsConfig.h
    resources/style/styleLoader.h
    modules/trajectory/trajectoryVisualizer.h
    modules/gcode/gcodeEditor.h
    modules/remote/remoteMonitor.h
    modules/plugin/pluginInterface.h
    modules/plugin/pluginManager.h
)

# UI 文件路径更新
set(UI_FILES
    resources/ui/mainwindow_refactored.ui
)

# 包含目录更新
include_directories(
    ${CMAKE_SOURCE_DIR}/zrcsGui/src
    ${CMAKE_SOURCE_DIR}/zrcsGui/modules
)
```

### 第 5 步: 更新 #include 语句

**在所有源文件中更新包含路径**

例如:
```cpp
// 旧的
#include "zmqClient.h"
#include "zrcsConfig.h"

// 新的
#include "communication/zmqClient.h"
#include "config/zrcsConfig.h"
```

## 整理优势

✅ **清晰的组织** - 按功能和类型分类  
✅ **易于维护** - 相关文件集中在一起  
✅ **易于扩展** - 新功能可以添加新模块  
✅ **易于导航** - 快速找到需要的文件  
✅ **专业结构** - 符合大型项目的组织方式  

## 推荐方案

**使用方案 A (按功能分类)** - 更清晰，更易于维护

---

**版本**: 2.0  
**完成日期**: 2026-03-12

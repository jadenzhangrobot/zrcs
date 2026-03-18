# UI 样式指南

## 文件结构

```
zrcsGui/
  ui/
    zrcsgui.ui                    # 原有 UI 文件
    mainwindow_refactored.ui      # 新的 UI 文件（Qt Designer 兼容）
  style/
    dark_theme.qss                # 深色主题样式表
    styleLoader.h                 # 样式加载器工具类
  zrcsStyles.h                    # 全局样式和颜色定义（内联样式）
```

## 使用 Qt Designer

### 打开 UI 文件

```bash
designer ui/mainwindow_refactored.ui
```

### 编辑流程

1. 打开 `.ui` 文件
2. 拖拽组件、调整布局、设置属性
3. Ctrl+S 保存
4. 重新编译

### UI 文件结构说明

`mainwindow_refactored.ui` 包含：

- **左侧面板** (500px 宽)
  - 全局状态监控分组框
  - 坐标显示滚动区
- **中间标签页**
  - 手动调试标签页
  - 报警与诊断标签页
  - I/O 控制标签页

### 在代码中使用 UI 文件

```cpp
#include "ui_mainwindow_refactored.h"

class MainWindowRefactored : public QMainWindow {
    Q_OBJECT

private:
    Ui::MainWindowRefactored ui;

public:
    MainWindowRefactored(QWidget *parent = nullptr) : QMainWindow(parent) {
        ui.setupUi(this);
    }
};
```

## 使用 QSS 样式文件

### 编辑样式

直接编辑 `style/dark_theme.qss` 文件，修改颜色和样式后保存。

### 在 Qt Designer 中预览

1. 打开 `.ui` 文件
2. 菜单 -> Form -> Preview Style Sheet
3. 选择 `.qss` 文件

## StyleLoader 使用方法

### 从文件加载

```cpp
#include "style/styleLoader.h"

QApplication app(argc, argv);
StyleLoader::loadStyleSheet("style/dark_theme.qss", &app);
```

### 从资源文件加载

```cpp
StyleLoader::loadStyleSheetFromResource(":/style/dark_theme.qss", &app);
```

### 使用内联样式

```cpp
app.setStyleSheet(StyleLoader::getDarkThemeStyleSheet());
```

### 运行时切换主题

```cpp
// 加载深色主题
StyleLoader::loadStyleSheet("style/dark_theme.qss", &app);

// 切换到其他主题
StyleLoader::loadStyleSheet("style/light_theme.qss", &app);
```

### 运行时重新加载样式

```cpp
QFile styleFile("style/dark_theme.qss");
styleFile.open(QFile::ReadOnly);
QString style = QLatin1String(styleFile.readAll());
styleFile.close();
qApp->setStyleSheet(style);
```

## 常见修改

### 修改背景颜色

编辑 `style/dark_theme.qss`:

```qss
QMainWindow {
    background-color: #0f0f0f;
}

QWidget {
    background-color: #1a1a1a;
}
```

### 修改文字颜色

```qss
QLabel {
    color: #CCCCCC;
}
```

### 修改按钮样式

```qss
QPushButton {
    background-color: #2a2a2a;
    color: #CCCCCC;
    border: 1px solid #555;
    border-radius: 3px;
    padding: 6px 12px;
}
```

### 添加自定义按钮样式

在 QSS 中定义：

```qss
QPushButton.customButton {
    background-color: #3a3a3a;
    color: #FFD700;
    border: 2px solid #FFD700;
}

QPushButton.customButton:hover {
    background-color: #4a4a4a;
}
```

在代码中使用：

```cpp
button->setObjectName("customButton");
```

## 颜色参考表

| 用途 | 颜色 | 十六进制 |
|------|------|---------|
| 背景 | 深灰 | #1a1a1a |
| 文字 | 浅灰 | #CCCCCC |
| 强调 | 金黄 | #FFD700 |
| 正常 | 绿色 | #00FF00 |
| 运行 | 蓝色 | #6496FF |
| 警告 | 黄色 | #FFC800 |
| 错误 | 红色 | #FF5050 |

## 最佳实践

### 1. 分离关注点

- UI 布局在 `.ui` 文件中
- 样式在 `.qss` 文件中
- 逻辑在 `.cpp` 文件中

### 2. 使用对象名称

```cpp
button->setObjectName("myButton");
```

在 QSS 中引用：

```qss
#myButton {
    background-color: #3a3a3a;
}
```

### 3. 使用类选择器

```cpp
button->setProperty("class", "greenButton");
```

在 QSS 中：

```qss
QPushButton.greenButton {
    background-color: #2a5a2a;
}
```

### 4. 版本控制

- 提交 `.ui` 文件
- 提交 `.qss` 文件
- 不提交生成的 `ui_*.h` 文件

## CMakeLists.txt 配置

确保 CMakeLists.txt 包含 UI 文件：

```cmake
set(UI_FILES
    ui/zrcsgui.ui
    ui/mainwindow_refactored.ui
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ui)
```

如果要在资源文件中包含样式，创建 `resources.qrc`:

```xml
<RCC>
    <qresource prefix="/style">
        <file>style/dark_theme.qss</file>
    </qresource>
</RCC>
```

在 CMakeLists.txt 中添加：

```cmake
set(RESOURCE_FILES resources.qrc)
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES} ${RESOURCE_FILES})
```

## 工作流程

1. **设计阶段** -- 使用 Qt Designer 编辑 `.ui` 文件，调整布局和属性
2. **样式阶段** -- 编辑 `.qss` 文件，调整颜色和字体
3. **开发阶段** -- 在代码中添加逻辑，连接信号槽
4. **测试阶段** -- 编译和运行，验证界面和功能

## 常见问题

**Q: 如何在 Qt Designer 中预览样式?**

A: 在 Qt Designer 中打开 `.ui` 文件，菜单 -> Form -> Preview Style Sheet，选择 `.qss` 文件。

**Q: 如何动态加载不同的样式?**

A: 使用 StyleLoader：
```cpp
StyleLoader::loadStyleSheet("style/dark_theme.qss", &app);
```

**Q: 如何在运行时修改样式?**

A: 直接读取 QSS 文件并应用：
```cpp
QFile styleFile("style/dark_theme.qss");
styleFile.open(QFile::ReadOnly);
qApp->setStyleSheet(QLatin1String(styleFile.readAll()));
styleFile.close();
```

**Q: 修改样式后需要重新编译吗?**

A: 如果使用外部 `.qss` 文件并通过 StyleLoader 加载，修改样式无需重新编译。如果修改的是 `zrcsStyles.h` 中的内联样式，则需要重新编译。

---

版本: 2.0
最后更新: 2026-03-18

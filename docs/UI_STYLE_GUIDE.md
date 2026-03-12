# UI 和样式文件使用指南

## 📁 文件结构

```
zrcsGui/
├── ui/
│   ├── zrcsgui.ui                    # 原有 UI 文件
│   └── mainwindow_refactored.ui      # 新的 UI 文件（Qt Designer 兼容）
│
├── style/
│   ├── dark_theme.qss                # 深色主题样式表
│   └── styleLoader.h                 # 样式加载器
│
└── [其他源文件]
```

## 🎨 使用方法

### 方法 1: 使用 Qt Designer 编辑 UI

1. **打开 Qt Designer**
   ```bash
   designer ui/mainwindow_refactored.ui
   ```

2. **编辑界面**
   - 拖拽组件
   - 调整布局
   - 设置属性

3. **保存**
   - Ctrl+S 保存

4. **重新编译**
   ```bash
   cd build
   mingw32-make
   ```

### 方法 2: 使用样式文件

#### 在代码中加载样式

```cpp
#include "style/styleLoader.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 方法 A: 从文件加载
    StyleLoader::loadStyleSheet("style/dark_theme.qss", &app);
    
    // 方法 B: 从资源加载
    // StyleLoader::loadStyleSheetFromResource(":/style/dark_theme.qss", &app);
    
    // 方法 C: 使用内联样式
    // app.setStyleSheet(StyleLoader::getDarkThemeStyleSheet());
    
    MainWindowRefactored window;
    window.show();
    
    return app.exec();
}
```

### 方法 3: 编辑 QSS 文件

1. **打开样式文件**
   ```
   style/dark_theme.qss
   ```

2. **修改颜色和样式**
   ```qss
   /* 修改背景色 */
   QMainWindow {
       background-color: #0f0f0f;  /* 改这里 */
   }
   ```

3. **保存并重新加载**

## 🎯 常见修改

### 修改背景颜色

编辑 `style/dark_theme.qss`:

```qss
QMainWindow {
    background-color: #0f0f0f;  /* 改为你想要的颜色 */
}

QWidget {
    background-color: #1a1a1a;  /* 改为你想要的颜色 */
}
```

### 修改文字颜色

```qss
QLabel {
    color: #CCCCCC;  /* 改为你想要的颜色 */
}
```

### 修改按钮样式

```qss
QPushButton {
    background-color: #2a2a2a;  /* 背景色 */
    color: #CCCCCC;             /* 文字色 */
    border: 1px solid #555;     /* 边框 */
    border-radius: 3px;         /* 圆角 */
    padding: 6px 12px;          /* 内边距 */
}
```

### 添加新的按钮样式

```qss
/* 在 dark_theme.qss 中添加 */
QPushButton.customButton {
    background-color: #3a3a3a;
    color: #FFD700;
    border: 2px solid #FFD700;
}

QPushButton.customButton:hover {
    background-color: #4a4a4a;
}
```

然后在代码中使用:

```cpp
button->setObjectName("customButton");
```

## 📝 UI 文件说明

### mainwindow_refactored.ui

这是一个标准的 Qt Designer UI 文件，包含：

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
        // 现在可以访问 ui 中的所有组件
        // ui.statusbar->showMessage("Hello");
    }
};
```

## 🔧 CMakeLists.txt 配置

确保 CMakeLists.txt 包含 UI 文件：

```cmake
set(UI_FILES
    ui/zrcsgui.ui
    ui/mainwindow_refactored.ui
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ui)
```

## 📦 资源文件配置

如果要在资源文件中包含样式，创建 `resources.qrc`:

```xml
<RCC>
    <qresource prefix="/style">
        <file>style/dark_theme.qss</file>
    </qresource>
</RCC>
```

然后在 CMakeLists.txt 中添加：

```cmake
set(RESOURCE_FILES
    resources.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES} ${RESOURCE_FILES})
```

## 🎨 颜色参考

| 用途 | 颜色 | 十六进制 |
|------|------|---------|
| 背景 | 深灰 | #1a1a1a |
| 文字 | 浅灰 | #CCCCCC |
| 强调 | 金黄 | #FFD700 |
| 正常 | 绿色 | #00FF00 |
| 运行 | 蓝色 | #6496FF |
| 警告 | 黄色 | #FFC800 |
| 错误 | 红色 | #FF5050 |

## 💡 最佳实践

1. **分离关注点**
   - UI 布局在 `.ui` 文件中
   - 样式在 `.qss` 文件中
   - 逻辑在 `.cpp` 文件中

2. **使用对象名称**
   ```cpp
   button->setObjectName("myButton");
   ```
   然后在 QSS 中引用：
   ```qss
   #myButton {
       background-color: #3a3a3a;
   }
   ```

3. **使用类选择器**
   ```cpp
   button->setProperty("class", "greenButton");
   ```
   然后在 QSS 中：
   ```qss
   QPushButton.greenButton {
       background-color: #2a5a2a;
   }
   ```

4. **版本控制**
   - 提交 `.ui` 文件
   - 提交 `.qss` 文件
   - 不提交生成的 `ui_*.h` 文件

## 🚀 工作流程

1. **设计阶段**
   - 使用 Qt Designer 编辑 `.ui` 文件
   - 调整布局和属性

2. **样式阶段**
   - 编辑 `.qss` 文件
   - 调整颜色和字体

3. **开发阶段**
   - 在代码中添加逻辑
   - 连接信号槽

4. **测试阶段**
   - 编译和运行
   - 验证界面和功能

## 📞 常见问题

### Q: 如何在 Qt Designer 中预览样式？

A: 在 Qt Designer 中：
1. 打开 `.ui` 文件
2. 菜单 → Form → Preview Style Sheet
3. 选择 `.qss` 文件

### Q: 如何动态加载不同的样式？

A: 使用 `StyleLoader`:

```cpp
// 加载深色主题
StyleLoader::loadStyleSheet("style/dark_theme.qss", &app);

// 或者切换到其他主题
StyleLoader::loadStyleSheet("style/light_theme.qss", &app);
```

### Q: 如何在运行时修改样式？

A: 直接修改 QSS 并重新加载：

```cpp
QFile styleFile("style/dark_theme.qss");
styleFile.open(QFile::ReadOnly);
QString style = QLatin1String(styleFile.readAll());
styleFile.close();
qApp->setStyleSheet(style);
```

## 📚 参考资源

- [Qt Designer 文档](https://doc.qt.io/qt-5/qtdesigner-manual.html)
- [Qt 样式表文档](https://doc.qt.io/qt-5/stylesheet.html)
- [Qt 颜色参考](https://doc.qt.io/qt-5/qcolor.html)

---

**版本**: 2.0  
**最后更新**: 2026-03-12

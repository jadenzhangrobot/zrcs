#ifndef STYLE_LOADER_H
#define STYLE_LOADER_H

#include <QString>
#include <QApplication>
#include <QFile>
#include <QDebug>

class StyleLoader {
public:
    /**
     * 加载 QSS 样式文件
     * @param filePath 样式文件路径
     * @param app Qt 应用程序指针
     * @return 是否加载成功
     */
    static bool loadStyleSheet(const QString &filePath, QApplication *app) {
        QFile styleFile(filePath);
        if (!styleFile.open(QFile::ReadOnly)) {
            qWarning() << "无法打开样式文件:" << filePath;
            return false;
        }
        
        QString style = QLatin1String(styleFile.readAll());
        styleFile.close();
        
        app->setStyleSheet(style);
        qDebug() << "样式文件加载成功:" << filePath;
        return true;
    }
    
    /**
     * 从资源文件加载样式
     * @param resourcePath 资源路径（如 :/style/dark_theme.qss）
     * @param app Qt 应用程序指针
     * @return 是否加载成功
     */
    static bool loadStyleSheetFromResource(const QString &resourcePath, QApplication *app) {
        QFile styleFile(resourcePath);
        if (!styleFile.open(QFile::ReadOnly)) {
            qWarning() << "无法打开资源文件:" << resourcePath;
            return false;
        }
        
        QString style = QLatin1String(styleFile.readAll());
        styleFile.close();
        
        app->setStyleSheet(style);
        qDebug() << "资源样式文件加载成功:" << resourcePath;
        return true;
    }
    
    /**
     * 获取深色主题样式表（内联）
     * @return 样式表字符串
     */
    static QString getDarkThemeStyleSheet() {
        return QString(
            "QMainWindow { background-color: #0f0f0f; }"
            "QWidget { background-color: #1a1a1a; color: #CCCCCC; }"
            "QMenuBar { background-color: #2a2a2a; color: #CCCCCC; border-bottom: 1px solid #444; }"
            "QMenuBar::item:selected { background-color: #3a3a3a; }"
            "QMenu { background-color: #2a2a2a; color: #CCCCCC; }"
            "QMenu::item:selected { background-color: #3a3a3a; }"
            "QStatusBar { background-color: #2a2a2a; color: #CCCCCC; border-top: 1px solid #444; }"
            "QTabWidget::pane { border: 1px solid #444; }"
            "QTabBar::tab { background-color: #2a2a2a; color: #CCCCCC; padding: 8px 20px; border: 1px solid #444; }"
            "QTabBar::tab:selected { background-color: #3a3a3a; color: #FFD700; border-bottom: 2px solid #FFD700; }"
            "QPushButton { background-color: #2a2a2a; color: #CCCCCC; border: 1px solid #555; border-radius: 3px; padding: 6px 12px; font-weight: bold; }"
            "QPushButton:hover { background-color: #3a3a3a; }"
            "QPushButton:pressed { background-color: #1a1a1a; }"
            "QLineEdit { background-color: #2a2a2a; color: #00FF00; border: 1px solid #555; border-radius: 3px; padding: 5px; font-family: monospace; }"
            "QSpinBox { background-color: #2a2a2a; color: #00FF00; border: 1px solid #555; border-radius: 3px; padding: 5px; }"
            "QComboBox { background-color: #2a2a2a; color: #CCCCCC; border: 1px solid #555; border-radius: 3px; padding: 5px; }"
            "QLabel { color: #CCCCCC; }"
            "QGroupBox { color: #FFD700; border: 1px solid #444; border-radius: 5px; padding-top: 10px; margin-top: 10px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }"
        );
    }
};

#endif // STYLE_LOADER_H

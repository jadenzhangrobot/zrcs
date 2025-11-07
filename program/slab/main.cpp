#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("SLAB控制系统");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("ZRCS");
    
    // 设置应用程序样式
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // 创建主窗口
    MainWindow window;
    
    // 显示窗口
    window.show();
    
    // 运行应用程序事件循环
    return app.exec();
}
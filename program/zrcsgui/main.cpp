#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include "mainwindow.h"
#include <QProcess>
int main(int argc, char *argv[])
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // 2. 允许图标使用高分率图片 (防止图标模糊)
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // 3. 针对 4K 屏幕可能的调整 (Qt 5.14+)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QCoreApplication::setAttribute(Qt::AA_Use96Dpi); // 可选，视情况而定
    // 或者设置缩放策略
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif
    QApplication app(argc, argv);
    QString appDir = QCoreApplication::applicationDirPath();
    QString programPath = appDir + "/zrcs.exe";
    QString plotPublisherPath = appDir + "/plotPublisher.exe";
    QProcess *zrcsProcess;
    zrcsProcess = new QProcess();
    zrcsProcess->start(programPath);
    QProcess *plot;
    plot = new QProcess();
    plot->start(plotPublisherPath);
    // 设置应用程序信息
    app.setApplicationName("ZRCS控制系统");
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
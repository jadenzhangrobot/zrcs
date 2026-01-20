#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include "mainwindow.h"
#include <QProcess>
#include "motion/velocityPlanner3D.h"
#ifndef Q_OS_WIN
#include <unistd.h>
#endif
int main(int argc, char *argv[])
{
    VelocityPlanner3D planner;
    planner.setConfig(100.0, 500.0, 0.0, 0.0);

    // 场景：
    // 1. 地面直线加速 (0,0,0) -> (50,0,0)
    // 2. 开始爬坡 (50,0,0) -> (100,0,50) [Z轴上升]
    // 3. 坡顶急转弯 (100,0,50) -> (100,50,50) [Z轴不变，XY平面转弯]
    
    planner.addPoint(0, 0, 0);      // 起点
    planner.addPoint(50, 0, 0);     // 地面点
    planner.addPoint(100, 0, 50);   // 坡顶路口 (入弯点)
    planner.addPoint(100, 5, 50);   // 坡顶弯道中 (距离很短，必须减速)
    planner.addPoint(100, 100, 50); // 出弯后直行

    if (planner.plan()) {
        planner.printReport();
    }

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
#ifdef Q_OS_WIN
    QString programPath = appDir + "/zrcs.exe";
    QString plotPublisherPath = appDir + "/plotPublisher.exe";
#else
    QString programPath = appDir + "/zrcs";
    QString plotPublisherPath = appDir + "/plotPublisher";
#endif
    QProcess *zrcsProcess;
    zrcsProcess = new QProcess(&app);
    zrcsProcess->start(programPath, QStringList());
    QProcess *plot;
    plot = new QProcess(&app);
    plot->start(plotPublisherPath, QStringList());

    // 确保主程序退出时子进程也能退出
    QObject::connect(&app, &QApplication::aboutToQuit, [zrcsProcess, plot](){
        zrcsProcess->kill();
        plot->kill();
    });

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
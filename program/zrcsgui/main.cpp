#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include "mainwindow.h"
#include <QProcess>
#include "motion/PathPreprocessor.h"
#ifndef Q_OS_WIN
#include <unistd.h>
#endif
int main(int argc, char *argv[])
{
    std::vector<Point3D> rawPoints = {
    {50.00, 65.00, 0}, {54.45, 64.12, 0}, {58.20, 61.35, 0}, {60.15, 57.12, 0},
    {59.80, 52.45, 0}, {64.12, 50.12, 0}, {70.50, 52.35, 0}, {78.20, 58.12, 0},
    {85.45, 62.45, 0}, {88.12, 55.20, 0}, {85.35, 45.15, 0}, {78.12, 38.45, 0},
    {68.50, 35.12, 0}, {62.15, 30.45, 0}, {65.45, 20.12, 0}, {72.12, 12.35, 0},
    {80.50, 8.12, 0},  {75.35, 2.45, 0},  {65.12, 5.12, 0},  {55.45, 12.35, 0},
    {50.00, 22.00, 0}, {44.55, 12.35, 0}, {34.88, 5.12, 0},  {24.65, 2.45, 0},
    {19.50, 8.12, 0},  {27.88, 12.35, 0}, {34.55, 20.12, 0}, {37.85, 30.45, 0},
    {31.50, 35.12, 0}, {21.88, 38.45, 0}, {14.65, 45.15, 0}, {11.88, 55.20, 0},
    {14.55, 62.45, 0}, {21.80, 58.12, 0}, {29.50, 52.35, 0}, {35.88, 50.12, 0},
    {40.20, 52.45, 0}, {39.85, 57.12, 0}, {41.80, 61.35, 0}, {45.55, 64.12, 0},
    {50.00, 65.00, 0}
};

    // 2. 实例化预处理器
    PathPreprocessor processor;

    // 3. 执行三次样条拟合与重采样
    // 建议 stepSize 设为 0.5mm 到 1.0mm，具体取决于你的加工精度需求
    double stepSize = 0.5; 
    std::vector<Point3D> smoothPath = processor.processWithSpline(rawPoints, stepSize);
    VelocityPlanner3D planner;
    planner.setConfig(100.0, 500.0, 0.0, 0.0);

    // 场景：
    // 1. 地面直线加速 (0,0,0) -> (50,0,0)
    // 2. 开始爬坡 (50,0,0) -> (100,0,50) [Z轴上升]
    // 3. 坡顶急转弯 (100,0,50) -> (100,50,50) [Z轴不变，XY平面转弯]
    for (const auto& pt : smoothPath) {
    planner.addPoint(pt.x, pt.y, pt.z);
}
  
    if (planner.plan()) {
        planner.printReport("3D_planning_report.txt", rawPoints);
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

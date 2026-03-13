#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include "mainwindow_refactored.h"

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

int main(int argc, char *argv[])
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication app(argc, argv);

    app.setApplicationName("ZRCS 工业运动控制系统");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("ZRCS");
    
    app.setStyle(QStyleFactory::create("Fusion"));
    
    MainWindowRefactored window;
    window.show();
    
    return app.exec();
}

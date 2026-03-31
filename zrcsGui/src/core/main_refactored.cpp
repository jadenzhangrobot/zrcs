#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include "mainwindow_refactored.h"
#include "bt_editor/bt_editor_base.h"

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    qRegisterMetaType<AbsBehaviorTree>();

    app.setApplicationName("ZRCS 工业运动控制系统");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("ZRCS");
    
    app.setStyle(QStyleFactory::create("Fusion"));
    
    MainWindowRefactored window;
    window.show();
    
    return app.exec();
}

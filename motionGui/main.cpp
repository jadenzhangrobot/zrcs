#include <QApplication>
#include <QStyleFactory>
#include "MotionMainWindow.h"

int main(int argc, char* argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("ZRCS Motion Control Panel");
    app.setApplicationVersion("1.0");
    app.setStyle(QStyleFactory::create("Fusion"));

    MotionMainWindow window;
    window.show();

    return app.exec();
}

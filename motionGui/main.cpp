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

// Qt6::Widgets on MinGW injects -Wl,-subsystem,windows which requires WinMain.
// Provide a WinMain bridge so we can keep using int main().
#ifdef _WIN32
#include <windows.h>
extern int __argc;
extern char** __argv;
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main(__argc, __argv);
}
#endif

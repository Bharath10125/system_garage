#include "ClockWindow.h"
#include <QApplication>
#include <QScreen>
#include <QFont>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FedoraClock");
    app.setApplicationVersion("1.0");

    // High DPI is on by default in Qt6
    ClockWindow window;
    window.showFullScreen();

    return app.exec();
}


#include "PulseWidget.h"
#include <QApplication>
#include <QScreen>

int main(int argc, char *argv[])
{
    // Force X11 backend (XWayland) because native Wayland completely 
    // prohibits programmatic window positioning for top-level windows.
    qputenv("QT_QPA_PLATFORM", "xcb");

    QApplication app(argc, argv);
    app.setApplicationName("PulseWidget");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("SystemGarage");

    // High-DPI is always enabled in Qt6

    PulseWidget widget;
    widget.show();

    return app.exec();
}

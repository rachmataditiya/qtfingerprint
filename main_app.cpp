#include "mainwindow_app.h"
#include <QApplication>
#include <QDebug>
#include <glib.h>

int main(int argc, char *argv[])
{
    // Disable fatal warnings from GLib/libfprint to prevent crashes on non-critical warnings
    g_log_set_always_fatal((GLogLevelFlags)G_LOG_LEVEL_ERROR);

    QApplication app(argc, argv);
    
    qInfo() << "=================================================";
    qInfo() << "U.are.U 4500 Fingerprint Application";
    qInfo() << "Using DigitalPersona Library v" << DigitalPersona::version();
    qInfo() << "=================================================";
    qInfo() << "";
    
    MainWindowApp window;
    window.show();
    
    return app.exec();
}


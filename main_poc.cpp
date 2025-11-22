#include "mainwindow_new.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application info
    app.setApplicationName("U.are.U 4500 Fingerprint PoC");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Fingerprint PoC");
    
    // Set modern style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    MainWindowNew window;
    window.show();
    
    return app.exec();
}


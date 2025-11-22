#include "mainwindow_app.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
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


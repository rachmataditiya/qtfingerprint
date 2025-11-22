QT += core gui widgets sql

CONFIG += c++17

TARGET = FingerprintApp
TEMPLATE = app

# Output directory
DESTDIR = bin

# Link to digitalpersonalib
LIBS += -L$$PWD/digitalpersonalib/lib -ldigitalpersona
INCLUDEPATH += $$PWD/digitalpersonalib/include
QMAKE_RPATHDIR += $$PWD/digitalpersonalib/lib

# Application sources
SOURCES += \
    main_app.cpp \
    mainwindow_app.cpp \
    database_manager.cpp

HEADERS += \
    mainwindow_app.h \
    database_manager.h

# Deploy library with app
QMAKE_POST_LINK += cp -f $$PWD/digitalpersonalib/lib/libdigitalpersona.so* $$PWD/bin/

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


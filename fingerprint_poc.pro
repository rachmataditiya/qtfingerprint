QT += core gui widgets sql

CONFIG += c++17

TARGET = FingerprintPoC
TEMPLATE = app

# libfprint (native ARM64) - only for linking, not headers
LIBS += -lfprint-2 -lgio-2.0 -lgobject-2.0 -lglib-2.0
INCLUDEPATH += /usr/include/libfprint-2 /usr/include/glib-2.0 /usr/lib/aarch64-linux-gnu/glib-2.0/include

# Source files
SOURCES += \
    main_poc.cpp \
    mainwindow_new.cpp \
    fingerprint_manager.cpp \
    database_manager.cpp

HEADERS += \
    mainwindow_new.h \
    fingerprint_manager.h \
    database_manager.h

# Output directory
DESTDIR = bin

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


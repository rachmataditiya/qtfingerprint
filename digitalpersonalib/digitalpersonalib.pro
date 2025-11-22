TEMPLATE = lib
TARGET = digitalpersona
VERSION = 1.0.0

CONFIG += qt shared c++17
QT += core sql

# Output directories
DESTDIR = $$PWD/lib
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc

# Library type
CONFIG += shared

# Define export macro
DEFINES += DIGITALPERSONA_LIBRARY

# Include paths
INCLUDEPATH += $$PWD/include
INCLUDEPATH += /usr/include/libfprint-2
INCLUDEPATH += /usr/include/glib-2.0
INCLUDEPATH += /usr/lib/aarch64-linux-gnu/glib-2.0/include

# Headers
HEADERS += \
    include/digitalpersona.h \
    include/digitalpersona_global.h \
    include/fingerprint_manager.h \
    include/database_manager.h

# Sources
SOURCES += \
    src/fingerprint_manager.cpp \
    src/database_manager.cpp

# Libraries
LIBS += -lfprint-2 -lgio-2.0 -lgobject-2.0 -lglib-2.0

# Install rules
headers.files = $$HEADERS
headers.path = /usr/local/include/digitalpersona

target.path = /usr/local/lib

INSTALLS += target headers

# Generate pkg-config file
CONFIG += create_pc create_prl no_install_prl

QMAKE_PKGCONFIG_NAME = digitalpersona
QMAKE_PKGCONFIG_DESCRIPTION = DigitalPersona Fingerprint Library
QMAKE_PKGCONFIG_PREFIX = /usr/local
QMAKE_PKGCONFIG_LIBDIR = /usr/local/lib
QMAKE_PKGCONFIG_INCDIR = /usr/local/include/digitalpersona
QMAKE_PKGCONFIG_VERSION = $$VERSION
QMAKE_PKGCONFIG_DESTDIR = pkgconfig


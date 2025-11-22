QT += core gui widgets sql concurrent

CONFIG += c++17

TARGET = FingerprintApp
TEMPLATE = app

# Output directory
DESTDIR = bin

# Link to digitalpersonalib (Binary)
INCLUDEPATH += $$PWD/digitalpersonalib/include

LIBS += -L$$PWD/digitalpersonalib/lib -ldigitalpersona
QMAKE_RPATHDIR += $$PWD/digitalpersonalib/lib

# Deploy library with app
# Only copy libfprint, digitalpersonalib is copied by the previous build step or manual setup
macx {
    # QMAKE_POST_LINK removed to avoid copy errors during build
}



# Link to libfprint (compiled from source) - macOS Only
macx {
    INCLUDEPATH += $$PWD/libfprint_repo/libfprint
    LIBS += -L$$PWD/libfprint_repo/builddir/libfprint -lfprint-2
    QMAKE_RPATHDIR += $$PWD/libfprint_repo/builddir/libfprint
    
    # Automate bundling of libraries and fixing load paths
    APP_BUNDLE_DIR = $$DESTDIR/$${TARGET}.app
    FRAMEWORKS_DIR = $$APP_BUNDLE_DIR/Contents/Frameworks
    EXECUTABLE_PATH = $$APP_BUNDLE_DIR/Contents/MacOS/$${TARGET}

    QMAKE_POST_LINK += mkdir -p $$FRAMEWORKS_DIR && \
                       cp -f $$PWD/digitalpersonalib/lib/libdigitalpersona.1.dylib $$FRAMEWORKS_DIR/ && \
                       install_name_tool -id @rpath/libdigitalpersona.1.dylib $$FRAMEWORKS_DIR/libdigitalpersona.1.dylib && \
                       cp -f $$PWD/libfprint_repo/builddir/libfprint/libfprint-2.2.dylib $$FRAMEWORKS_DIR/ && \
                       install_name_tool -id @rpath/libfprint-2.2.dylib $$FRAMEWORKS_DIR/libfprint-2.2.dylib && \
                       install_name_tool -change libdigitalpersona.1.dylib @rpath/libdigitalpersona.1.dylib $$EXECUTABLE_PATH
}

# Libfprint Dependencies (Homebrew on macOS)
macx {
    INCLUDEPATH += /opt/homebrew/include/glib-2.0 \
                   /opt/homebrew/lib/glib-2.0/include \
                   /opt/homebrew/include/gusb-1 \
                   /opt/homebrew/include/libusb-1.0 \
                   /opt/homebrew/include/pixman-1 \
                   /opt/homebrew/include/json-glib-1.0

    LIBS += -L/opt/homebrew/lib \
            -L/opt/homebrew/Cellar/libgusb/0.4.9/lib \
            -L/opt/homebrew/Cellar/libusb/1.0.29/lib \
            -L/opt/homebrew/Cellar/json-glib/1.10.8/lib \
            -L/opt/homebrew/Cellar/glib/2.86.1/lib \
            -L/opt/homebrew/opt/gettext/lib \
            -lglib-2.0 -lgobject-2.0 -lgio-2.0 \
            -lgusb -lusb-1.0 -lpixman-1 -lcairo -ljson-glib-1.0 -lintl
}

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += glib-2.0 gusb libusb-1.0 pixman-1 libfprint-2
}



# Application sources
SOURCES += \
    main_app.cpp \
    mainwindow_app.cpp \
    database_manager.cpp \
    database_config_dialog.cpp \
    migration_manager.cpp

HEADERS += \
    mainwindow_app.h \
    database_manager.h \
    database_config_dialog.h \
    migration_manager.h

RESOURCES += migrations.qrc



# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


QT += core gui widgets concurrent network

CONFIG += c++17

TARGET = FingerprintApp
TEMPLATE = app

# Output directory
DESTDIR = bin

# Link to digitalpersonalib (Binary)
INCLUDEPATH += $$PWD/digitalpersonalib/include

# Android: use architecture-specific library name
android {
    LIBS += -L$$PWD/digitalpersonalib/lib -ldigitalpersona_arm64-v8a
} else {
    LIBS += -L$$PWD/digitalpersonalib/lib -ldigitalpersona
}
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
                       install_name_tool -change libdigitalpersona.1.dylib @rpath/libdigitalpersona.1.dylib $$EXECUTABLE_PATH && \
                       /opt/homebrew/opt/qt/bin/macdeployqt $$APP_BUNDLE_DIR -verbose=0 && \
                       install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtWidgets.framework/Versions/A/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/A/QtWidgets $$EXECUTABLE_PATH && \
                       install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtGui.framework/Versions/A/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/A/QtGui $$EXECUTABLE_PATH && \
                       install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtCore.framework/Versions/A/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/A/QtCore $$EXECUTABLE_PATH && \
                       install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtNetwork.framework/Versions/A/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/A/QtNetwork $$EXECUTABLE_PATH && \
                       install_name_tool -change /opt/homebrew/opt/qtbase/lib/QtConcurrent.framework/Versions/A/QtConcurrent @executable_path/../Frameworks/QtConcurrent.framework/Versions/A/QtConcurrent $$EXECUTABLE_PATH && \
                       find $$FRAMEWORKS_DIR -name "*.framework" -type d -exec codesign --force --sign - --entitlements $$PWD/entitlements.plist {}/Versions/A/* \; 2>&1 | grep -v "replacing existing signature" ; true && \
                       codesign --force --sign - --entitlements $$PWD/entitlements.plist $$EXECUTABLE_PATH 2>&1 | grep -v "replacing existing signature" ; true && \
                       codesign --force --sign - --entitlements $$PWD/entitlements.plist $$APP_BUNDLE_DIR 2>&1 | grep -v "replacing existing signature" ; true
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

unix:!macx:!android {
    CONFIG += link_pkgconfig
    # Only libfprint-2 is usually needed, it pulls other deps automatically.
    # We include glib-2.0 because we use GMainLoop/GError in our code.
    PKGCONFIG += libfprint-2 glib-2.0
    
    # Ensure custom library can be found at runtime
    QMAKE_LFLAGS += -Wl,-rpath,\'\$$ORIGIN/../digitalpersonalib/lib\'
    QMAKE_LFLAGS += -Wl,-rpath,\'\$$ORIGIN/lib\'
}

# Android settings
android {
    # Android NDK prefix (where libfprint was built)
    ANDROID_PREFIX = $$PWD/android-ndk-prefix
    
    # Link to libfprint and dependencies for Android
    INCLUDEPATH += $$ANDROID_PREFIX/include \
                   $$ANDROID_PREFIX/include/libfprint-2 \
                   $$ANDROID_PREFIX/include/glib-2.0 \
                   $$ANDROID_PREFIX/lib/glib-2.0/include \
                   $$ANDROID_PREFIX/include/gusb-1 \
                   $$ANDROID_PREFIX/include/libusb-1.0 \
                   $$ANDROID_PREFIX/include/json-glib-1.0
    
    LIBS += -L$$ANDROID_PREFIX/lib \
            -lfprint-2 \
            -lglib-2.0 \
            -lgobject-2.0 \
            -lgio-2.0 \
            -lgusb \
            -lusb-1.0 \
            -ljson-glib-1.0 \
            -lffi \
            -lssl \
            -lcrypto
    
    # Copy native libraries to Android package
    ANDROID_EXTRA_LIBS = $$ANDROID_PREFIX/lib/libfprint-2.so \
                         $$ANDROID_PREFIX/lib/libglib-2.0.so \
                         $$ANDROID_PREFIX/lib/libgobject-2.0.so \
                         $$ANDROID_PREFIX/lib/libgio-2.0.so \
                         $$ANDROID_PREFIX/lib/libgusb.so \
                         $$ANDROID_PREFIX/lib/libusb-1.0.so \
                         $$ANDROID_PREFIX/lib/libjson-glib-1.0.so \
                         $$ANDROID_PREFIX/lib/libffi.so \
                         $$ANDROID_PREFIX/lib/libssl.so \
                         $$ANDROID_PREFIX/lib/libcrypto.so \
                         $$PWD/digitalpersonalib/lib/libdigitalpersona.so
    
    # Android deployment settings
    ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
    ANDROID_MIN_SDK_VERSION = 28
    ANDROID_TARGET_SDK_VERSION = 36
    
    # Permissions for USB access
    ANDROID_MANIFEST_PERMISSIONS += android.permission.USB_PERMISSION
}



# Application sources
SOURCES += \
    main_app.cpp \
    mainwindow_app.cpp \
    backend_client.cpp \
    backend_config_dialog.cpp

HEADERS += \
    mainwindow_app.h \
    backend_client.h \
    backend_config_dialog.h

RESOURCES += migrations.qrc



# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


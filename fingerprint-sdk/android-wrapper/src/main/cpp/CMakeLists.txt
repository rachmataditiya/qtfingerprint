cmake_minimum_required(VERSION 3.18.1)
project("arkana_fprint")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
set(ANDROID_NDK_PREFIX "${CMAKE_SYSROOT}/usr")

# Include paths for libfprint and dependencies
include_directories(
    ${CMAKE_SYSROOT}/usr/include
    ${CMAKE_SYSROOT}/usr/include/libfprint-2
    ${CMAKE_SYSROOT}/usr/include/glib-2.0
    ${CMAKE_SYSROOT}/usr/include/glib-2.0/glib
    ${CMAKE_SYSROOT}/usr/include/gobject-2.0
    ${CMAKE_SYSROOT}/usr/include/gobject-2.0/gobject
    ${CMAKE_SYSROOT}/usr/include/gio-2.0
    ${CMAKE_SYSROOT}/usr/include/gio-2.0/gio
    ${CMAKE_SYSROOT}/usr/include/gusb-1
    ${CMAKE_SYSROOT}/usr/include/gusb-1/gusb
    ${CMAKE_SYSROOT}/usr/include/libusb-1.0
    ${CMAKE_SYSROOT}/usr/include/json-glib-1.0
    ${CMAKE_SYSROOT}/usr/include/json-glib-1.0/json-glib
    ${CMAKE_SYSROOT}/usr/lib/glib-2.0/include
    ${CMAKE_SYSROOT}/usr/lib/gobject-2.0/include
    ${CMAKE_SYSROOT}/usr/lib/gio-2.0/include
    # Include existing FingerprintCapture
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libdigitalpersona/app/src/main/cpp
)

# Source files
add_library(arkana_fprint SHARED
    arkana_fprint.cpp
    # Include existing FingerprintCapture
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../libdigitalpersona/app/src/main/cpp/fingerprint_capture.cpp
)

# Link against libfprint and dependencies
target_link_libraries(arkana_fprint
    # libfprint
    ${CMAKE_SYSROOT}/usr/lib/libfprint-2.so
    # GLib
    ${CMAKE_SYSROOT}/usr/lib/libglib-2.0.so
    ${CMAKE_SYSROOT}/usr/lib/libgobject-2.0.so
    ${CMAKE_SYSROOT}/usr/lib/libgio-2.0.so
    # GUSB
    ${CMAKE_SYSROOT}/usr/lib/libgusb.so
    # libusb
    ${CMAKE_SYSROOT}/usr/lib/libusb-1.0.so
    # json-glib
    ${CMAKE_SYSROOT}/usr/lib/libjson-glib-1.0.so
    # libffi
    ${CMAKE_SYSROOT}/usr/lib/libffi.so
    # OpenSSL
    ${CMAKE_SYSROOT}/usr/lib/libcrypto.so
    ${CMAKE_SYSROOT}/usr/lib/libssl.so
    # Android log
    log
)


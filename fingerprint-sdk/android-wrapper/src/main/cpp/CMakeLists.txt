cmake_minimum_required(VERSION 3.18.1)
project("arkana_fprint")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to Android NDK prefix (where libfprint and dependencies are installed)
# CMAKE_SOURCE_DIR is fingerprint-sdk/android-wrapper/src/main/cpp
get_filename_component(ANDROID_NDK_PREFIX "${CMAKE_SOURCE_DIR}/../../../../../android-ndk-prefix" ABSOLUTE)

# Include paths for libfprint and dependencies
include_directories(
    ${ANDROID_NDK_PREFIX}/include
    ${ANDROID_NDK_PREFIX}/include/libfprint-2
    ${ANDROID_NDK_PREFIX}/include/glib-2.0
    ${ANDROID_NDK_PREFIX}/lib/glib-2.0/include
    ${ANDROID_NDK_PREFIX}/include/gusb-1
    ${ANDROID_NDK_PREFIX}/include/json-glib-1.0
    ${ANDROID_NDK_PREFIX}/include/libusb-1.0
    # Include local FingerprintCapture (SDK's own copy)
    ${CMAKE_SOURCE_DIR}
)

# Source files
add_library(arkana_fprint SHARED
    arkana_fprint.cpp
    # Use SDK's own copy of FingerprintCapture
    fingerprint_capture.cpp
)

# Link directories
link_directories(${ANDROID_NDK_PREFIX}/lib)

# Link against libfprint and dependencies
target_link_libraries(arkana_fprint
    # Android system libraries
    android
    log
    # libfprint and dependencies (using full paths)
    ${ANDROID_NDK_PREFIX}/lib/libfprint-2.so
    ${ANDROID_NDK_PREFIX}/lib/libglib-2.0.so
    ${ANDROID_NDK_PREFIX}/lib/libgobject-2.0.so
    ${ANDROID_NDK_PREFIX}/lib/libgio-2.0.so
    ${ANDROID_NDK_PREFIX}/lib/libgusb.so
    ${ANDROID_NDK_PREFIX}/lib/libusb-1.0.so
    ${ANDROID_NDK_PREFIX}/lib/libjson-glib-1.0.so
    ${ANDROID_NDK_PREFIX}/lib/libffi.so
    ${ANDROID_NDK_PREFIX}/lib/libssl.so
    ${ANDROID_NDK_PREFIX}/lib/libcrypto.so
    dl
    m
)

# Copy libfprint and dependencies to jniLibs
file(GLOB LIBFPRINT_LIBS "${ANDROID_NDK_PREFIX}/lib/*.so")
foreach(lib ${LIBFPRINT_LIBS})
    get_filename_component(libname ${lib} NAME)
    add_custom_command(TARGET arkana_fprint POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${lib}
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${libname}
    )
endforeach()


# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("libdigitalpersona")

# Path to Android NDK prefix (where libfprint and dependencies are installed)
# This should be set to the android-ndk-prefix directory created by build scripts
# CMAKE_SOURCE_DIR is libdigitalpersona/app/src/main/cpp, so we need to go up 5 levels
get_filename_component(ANDROID_NDK_PREFIX "${CMAKE_SOURCE_DIR}/../../../../../android-ndk-prefix" ABSOLUTE)

# Include directories for libfprint and dependencies
include_directories(
    ${ANDROID_NDK_PREFIX}/include
    ${ANDROID_NDK_PREFIX}/include/libfprint-2
    ${ANDROID_NDK_PREFIX}/include/glib-2.0
    ${ANDROID_NDK_PREFIX}/lib/glib-2.0/include
)

# Debug: Print include paths
message(STATUS "ANDROID_NDK_PREFIX: ${ANDROID_NDK_PREFIX}")
message(STATUS "Include paths:")
message(STATUS "  ${ANDROID_NDK_PREFIX}/include")
message(STATUS "  ${ANDROID_NDK_PREFIX}/include/libfprint-2")
message(STATUS "  ${ANDROID_NDK_PREFIX}/include/glib-2.0")
message(STATUS "  ${ANDROID_NDK_PREFIX}/lib/glib-2.0/include")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.
add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        native-lib.cpp
        fingerprint_manager_android.cpp
        fingerprint_capture.cpp)

# Find JNI
find_library(jnigraphics-lib jnigraphics)

# Link directories for libfprint and dependencies
link_directories(${ANDROID_NDK_PREFIX}/lib)

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # Android system libraries
        android
        log
        ${jnigraphics-lib}
        # libfprint and dependencies
        fprint-2
        glib-2.0
        gobject-2.0
        gio-2.0
        gusb
        usb-1.0
        pixman-1
        ffi
        dl
        m)

# Copy libfprint and dependencies to jniLibs
# This ensures they are packaged with the APK
file(GLOB LIBFPRINT_LIBS "${ANDROID_NDK_PREFIX}/lib/*.so")
foreach(lib ${LIBFPRINT_LIBS})
    get_filename_component(libname ${lib} NAME)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${lib}
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${libname}
    )
endforeach()
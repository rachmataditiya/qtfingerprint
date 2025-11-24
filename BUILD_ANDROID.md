# Building libfprint for Android NDK

Complete guide untuk build libfprint dan dependencies untuk Android NDK.

## Prerequisites

1. **Android NDK** - Install via Android Studio SDK Manager atau download manual
   - Default location: `~/Library/Android/sdk/ndk/<version>` (macOS)
   - Atau set `ANDROID_NDK` environment variable

2. **Build Tools**
   ```bash
   # macOS
   brew install meson ninja pkg-config
   pip3 install packaging
   
   # Linux
   sudo apt install meson ninja-build pkg-config python3-pip
   pip3 install packaging
   ```

3. **Qt 6** (untuk build Qt application) - **REQUIRED**
   - Install Qt 6 via Qt Installer atau Homebrew: `brew install qt@6`
   - Verify: `qmake6 -query QT_VERSION` should show 6.x.x
   - Configure Qt Creator untuk Android (SDK, NDK, JDK)
   - **Note:** Script menggunakan `qmake6` secara eksplisit

## Quick Start

### 1. Build libfprint untuk Android

```bash
# Build untuk arm64-v8a (default)
./build_libfprint_android_complete.sh

# Atau specify architecture dan API level
./build_libfprint_android_complete.sh arm64-v8a 29
./build_libfprint_android_complete.sh armeabi-v7a 21
./build_libfprint_android_complete.sh x86_64 29
```

Script ini akan:
- Download dan build semua dependencies (libusb, libffi, GLib, json-glib, libgusb, OpenSSL)
- Build libfprint dengan driver `uru4000`
- Install semua libraries ke `android-ndk-prefix/`

**Output:**
```
android-ndk-prefix/
├── lib/
│   ├── libfprint-2.so          # Main library
│   ├── libglib-2.0.so          # GLib
│   ├── libgobject-2.0.so       # GObject
│   ├── libgio-2.0.so           # GIO
│   ├── libgusb.so              # GUSB
│   ├── libusb-1.0.so           # libusb
│   ├── libjson-glib-1.0.so     # json-glib
│   ├── libffi.so               # libffi
│   ├── libssl.so               # OpenSSL
│   └── libcrypto.so            # OpenSSL crypto
├── include/
│   ├── libfprint-2/            # libfprint headers
│   ├── glib-2.0/               # GLib headers
│   └── ...
└── lib/pkgconfig/              # pkg-config files
```

### 2. Build Qt Application untuk Android

```bash
# Build Qt app untuk Android
./build_qt_android.sh arm64-v8a 29
```

Atau via Qt Creator:
1. Open `fingerprint_app.pro` di Qt Creator
2. Select Android kit (arm64-v8a)
3. Configure build settings:
   - Android NDK: Set ke NDK path
   - Android SDK: Set ke SDK path
   - Minimum SDK: 21
   - Target SDK: 29
4. Build & Run

## Architecture Support

Script mendukung multiple architectures:

- **arm64-v8a** (64-bit ARM) - Recommended untuk modern devices
- **armeabi-v7a** (32-bit ARM) - Legacy support
- **x86_64** (64-bit x86) - Emulator support

## Configuration

### Environment Variables

```bash
# Set Android NDK path (optional, auto-detected)
export ANDROID_NDK=/path/to/android-ndk

# Set architecture (optional, default: arm64-v8a)
export ANDROID_ARCH=arm64-v8a

# Set API level (optional, default: 29)
export ANDROID_API_LEVEL=29
```

### Custom Installation Prefix

Edit script dan ubah:
```bash
ANDROID_PREFIX="${SCRIPT_DIR}/android-ndk-prefix"
```

## Troubleshooting

### Error: "C compiler cannot create executables"

- Pastikan Android NDK path benar
- Check `ANDROID_NDK` environment variable
- Verify toolchain exists: `$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin/`

### Error: "Python packaging module not found"

```bash
pip3 install packaging
# atau dengan --break-system-packages jika perlu
pip3 install --break-system-packages packaging
```

### Error: "meson: command not found"

```bash
# macOS
brew install meson

# Linux
sudo apt install meson
```

### Error: "OpenSSL is required for uru4000"

Script sudah include OpenSSL build. Jika masih error:
- Check OpenSSL build logs di `build-android-${ARCH}/openssl/`
- Verify `libssl.so` dan `libcrypto.so` ada di `android-ndk-prefix/lib/`

### Qt Build Error: "Cannot find libfprint-2.so"

1. Pastikan `build_libfprint_android_complete.sh` sudah dijalankan
2. Check `android-ndk-prefix/lib/libfprint-2.so` exists
3. Verify `ANDROID_PREFIX` di `.pro` file mengarah ke `android-ndk-prefix`

## Using in Other Projects

### CMakeLists.txt

```cmake
# Set Android prefix
set(ANDROID_PREFIX "${CMAKE_SOURCE_DIR}/android-ndk-prefix")

# Include directories
target_include_directories(your_target PRIVATE
    ${ANDROID_PREFIX}/include
    ${ANDROID_PREFIX}/include/libfprint-2
    ${ANDROID_PREFIX}/include/glib-2.0
    ${ANDROID_PREFIX}/lib/glib-2.0/include
)

# Link libraries
target_link_libraries(your_target
    ${ANDROID_PREFIX}/lib/libfprint-2.so
    ${ANDROID_PREFIX}/lib/libglib-2.0.so
    ${ANDROID_PREFIX}/lib/libgobject-2.0.so
    ${ANDROID_PREFIX}/lib/libgio-2.0.so
    ${ANDROID_PREFIX}/lib/libgusb.so
    ${ANDROID_PREFIX}/lib/libusb-1.0.so
    ${ANDROID_PREFIX}/lib/libjson-glib-1.0.so
    ${ANDROID_PREFIX}/lib/libffi.so
    ${ANDROID_PREFIX}/lib/libssl.so
    ${ANDROID_PREFIX}/lib/libcrypto.so
)

# Copy libraries to jniLibs
file(GLOB LIBFPRINT_LIBS "${ANDROID_PREFIX}/lib/*.so")
file(COPY ${LIBFPRINT_LIBS} DESTINATION ${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/)
```

### Android Gradle (build.gradle.kts)

```kotlin
android {
    // ...
    
    sourceSets {
        getByName("main") {
            jniLibs.srcDirs("src/main/jniLibs")
        }
    }
}
```

## Standalone Script Usage

Script `build_libfprint_android_complete.sh` bisa digunakan di project lain:

1. Copy script ke project root
2. Clone libfprint repository:
   ```bash
   git clone https://gitlab.freedesktop.org/libfprint/libfprint.git libfprint_repo
   ```
3. Run script:
   ```bash
   ./build_libfprint_android_complete.sh
   ```
4. Libraries akan diinstall ke `android-ndk-prefix/` di project root

## Notes

- Build time: ~10-30 menit tergantung CPU dan internet speed
- Disk space: ~500MB untuk source + build artifacts
- Final libraries: ~50MB total
- Driver: Hanya `uru4000` driver yang di-build (untuk DigitalPersona U.are.U 4000)
- Features disabled: udev rules, documentation, tests, introspection

## References

- [libfprint Documentation](https://gitlab.freedesktop.org/libfprint/libfprint)
- [Android NDK Documentation](https://developer.android.com/ndk)
- [Qt for Android](https://doc.qt.io/qt-6/android.html)


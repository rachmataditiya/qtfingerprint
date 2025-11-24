# Build Instructions

How to build the Fingerprint SDK from source.

## Prerequisites

### For Rust Service

- Rust 1.70+ (install from https://rustup.rs/)
- Android NDK 25.1.8937393+
- Cargo (comes with Rust)

### For Android Wrapper

- Android Studio or command line tools
- Gradle 8.0+
- Android SDK
- Kotlin 1.9+

### For JNI Bridge

- CMake 3.18+
- Android NDK
- Clang compiler

## Building

### 1. Build Rust Service

```bash
cd fingerprint-sdk/rust-jni/rust-service

# Build for Android arm64-v8a
cargo build --target aarch64-linux-android --release
```

### 2. Build JNI Bridge

```bash
cd fingerprint-sdk/rust-jni/jni-bridge

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_NDK=/path/to/ndk \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/ndk/build/cmake/android.toolchain.cmake

# Build
cmake --build .
```

### 3. Build Android Wrapper

```bash
cd fingerprint-sdk/android-wrapper

# Build AAR
./gradlew assembleRelease

# Output: android-wrapper/build/outputs/aar/fingerprint-sdk-release.aar
```

## Integration

### Option 1: Use AAR

1. Copy `fingerprint-sdk-release.aar` to your project's `libs/` directory
2. Add to `build.gradle`:
   ```gradle
   dependencies {
       implementation files('libs/fingerprint-sdk-release.aar')
   }
   ```

### Option 2: Include as Module

1. Copy `android-wrapper/` to your project
2. Add to `settings.gradle`:
   ```gradle
   include ':fingerprint-sdk'
   project(':fingerprint-sdk').projectDir = new File('path/to/android-wrapper')
   ```
3. Add dependency:
   ```gradle
   dependencies {
       implementation project(':fingerprint-sdk')
   }
   ```

## Native Libraries

The SDK requires native libraries from `uareu-android-libs`:

1. Copy `.so` files to `android-wrapper/src/main/jniLibs/arm64-v8a/`:
   - `libfprint-2.so`
   - `libgusb.so`
   - `libusb-1.0.so`
   - `libglib-2.0.so`
   - `libgobject-2.0.so`
   - `libgio-2.0.so`
   - `libjson-glib-1.0.so`
   - `libffi.so`
   - `libcrypto.so`
   - `libssl.so`
   - `libfingerprint-sdk-service.so` (from Rust build)

## Testing

```bash
# Run unit tests
cd android-wrapper
./gradlew test

# Run integration tests
./gradlew connectedAndroidTest
```

## Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for common build issues.


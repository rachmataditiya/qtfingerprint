# Implementation Plan

## Overview

This document outlines the implementation plan for the Fingerprint SDK. The SDK is **hardware-focused** and runs entirely on Android. Backend API is a separate service.

## Architecture

### SDK Components (On Device)

```
┌─────────────────────────────────────────────────────────┐
│           Android App (Kotlin/Java)                     │
│  FingerprintSDK.enrollFingerprint(userId)              │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│      Android Wrapper (Kotlin)                          │
│  - USB permission management                           │
│  - HTTP client for backend API                          │
│  - Thread management                                   │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│         JNI Bridge (C/C++)                             │
│  Java_com_arkana_fingerprintsdk_FingerprintSDK_        │
│    nativeEnroll(JNIEnv*, jclass, jint userId)          │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│      C++ FingerprintCapture                            │
│  - Uses existing code from libdigitalpersona/           │
│  - libfprint integration                               │
│  - USB device communication                            │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│         Native Libraries                                │
│  - libfprint                                           │
│  - libgusb, libusb                                     │
│  - GLib, json-glib, etc.                              │
└─────────────────────────────────────────────────────────┘
```

### Backend API (Separate Service)

```
┌─────────────────────────────────────────────────────────┐
│      Backend API (Rust Axum)                           │
│  - Template storage                                    │
│  - User management                                     │
│  - REST API endpoints                                 │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│         Database                                       │
│  - PostgreSQL/SQLite                                  │
│  - User data                                          │
│  - Fingerprint templates                              │
└─────────────────────────────────────────────────────────┘
```

## Implementation Steps

### Step 1: Reuse Existing C++ Code

**Location**: `libdigitalpersona/app/src/main/cpp/`

**Files to reuse**:
- `fingerprint_capture.cpp` / `fingerprint_capture.h`
- `fingerprint_manager_android.cpp` / `fingerprint_manager_android.h`
- `native-lib.cpp` (JNI functions)

**Modifications needed**:
- Clean up JNI interface
- Add methods for SDK operations
- Ensure proper error handling

### Step 2: Create JNI Bridge for SDK

**Location**: `fingerprint-sdk/jni-bridge/`

**File**: `jni_bridge.cpp`

```cpp
#include <jni.h>
#include "fingerprint_capture.h"

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeEnroll(
    JNIEnv* env, jclass clazz, jint userId) {
    
    // Use existing FingerprintCapture
    FingerprintCapture* capture = new FingerprintCapture();
    if (!capture->initialize()) {
        return JNI_FALSE;
    }
    
    // Set USB file descriptor (from Android)
    // capture->setUsbFileDescriptor(fd);
    
    // Open device
    if (!capture->openDevice(0)) {
        return JNI_FALSE;
    }
    
    // Capture and create template
    std::vector<uint8_t> templateData;
    // ... enrollment logic ...
    
    // Return template to Kotlin layer
    // Kotlin layer will send to backend API
    
    return JNI_TRUE;
}

// ... other JNI functions

}
```

### Step 3: Android Wrapper with HTTP Client

**Location**: `fingerprint-sdk/android-wrapper/`

**File**: `FingerprintSDK.kt`

```kotlin
class FingerprintSDK private constructor(
    private val context: Context,
    private val config: Config
) {
    private val httpClient = OkHttpClient()
    private val backendUrl = config.backendUrl
    
    fun enrollFingerprint(userId: Int) {
        executor.execute {
            // 1. Call native enroll
            val template = nativeEnroll(userId)
            
            // 2. Send template to backend
            val response = httpClient.newCall(
                Request.Builder()
                    .url("$backendUrl/users/$userId/fingerprint")
                    .post(RequestBody.create(
                        MediaType.get("application/json"),
                        jsonObjectOf("template" to base64Encode(template))
                    ))
                    .build()
            ).execute()
            
            // 3. Call callback
            if (response.isSuccessful) {
                callback?.onEnrollSuccess(userId)
            } else {
                callback?.onEnrollError(userId, "Failed to store template")
            }
        }
    }
    
    fun verifyFingerprint(userId: Int) {
        executor.execute {
            // 1. Get template from backend
            val template = httpClient.newCall(
                Request.Builder()
                    .url("$backendUrl/users/$userId/fingerprint")
                    .get()
                    .build()
            ).execute().body()?.string()?.let { json ->
                // Parse JSON and decode template
                base64Decode(json.getString("template"))
            }
            
            // 2. Call native verify
            val score = nativeVerify(template)
            
            // 3. Call callback
            if (score >= config.matchThreshold) {
                callback?.onVerifySuccess(userId, score)
            } else {
                callback?.onVerifyError(userId, "Verification failed")
            }
        }
    }
    
    // ... other methods
}
```

### Step 4: Build Configuration

**CMakeLists.txt** for JNI bridge:
```cmake
cmake_minimum_required(VERSION 3.18)
project(fingerprint-sdk-native)

add_library(fingerprint-sdk-native SHARED
    jni_bridge.cpp
)

target_link_libraries(fingerprint-sdk-native
    fingerprint-capture  # From libdigitalpersona
    fprint-2
    gusb
    usb-1.0
    glib-2.0
    gobject-2.0
    gio-2.0
    json-glib-1.0
    log
)
```

**build.gradle.kts** for Android wrapper:
```kotlin
android {
    // ...
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
        }
    }
}

dependencies {
    implementation("com.squareup.okhttp3:okhttp:4.12.0")
    implementation("org.jetbrains.kotlinx:kotlinx-serialization-json:1.6.0")
}
```

### Step 5: Integration with Existing Code

**Reuse**:
- `FingerprintCapture` from `libdigitalpersona/`
- USB permission handling from `MainActivity.kt`
- Backend API endpoints (already exist in `middleware/rust-api/`)

**New**:
- Clean Kotlin wrapper
- JNI bridge for SDK
- HTTP client integration

## File Structure

```
fingerprint-sdk/
├── README.md
├── android-wrapper/
│   ├── src/main/java/com/arkana/fingerprintsdk/
│   │   ├── FingerprintSDK.kt
│   │   └── exceptions/
│   ├── src/main/cpp/
│   │   ├── jni_bridge.cpp
│   │   └── CMakeLists.txt
│   └── build.gradle.kts
├── docs/
│   ├── ARCHITECTURE.md
│   ├── API_REFERENCE.md
│   ├── EXAMPLES.md
│   └── BUILD.md
└── examples/
    └── BasicUsage.kt
```

## Dependencies

### Android Wrapper
- OkHttp: HTTP client for backend API
- Kotlinx Serialization: JSON parsing

### Native
- Existing C++ code from `libdigitalpersona/`
- libfprint and dependencies from `uareu-android-libs/`

## Next Steps

1. ✅ Remove Rust from SDK
2. Create JNI bridge using existing C++ code
3. Implement Android wrapper with HTTP client
4. Test end-to-end
5. Create build scripts
6. Document API

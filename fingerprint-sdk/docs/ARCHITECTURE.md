# SDK Architecture

## Overview

The Fingerprint SDK is designed with a clean layered architecture that abstracts all the complexity of USB communication and libfprint integration. The SDK runs entirely on the Android device and communicates with a separate backend API for template storage.

## Architecture Decision

**Important**: 
- **SDK**: Runs on Android device, handles USB hardware and fingerprint operations
- **Backend API**: Separate service (Rust Axum) for template storage and user management
- **No Rust in SDK**: SDK uses only Kotlin/Java and C++

## Layer Breakdown

### 1. Android Application Layer (Kotlin/Java)

**Responsibility**: Application logic and UI

**What it does**:
- Calls SDK methods
- Handles UI updates
- Manages application state

**What it doesn't know**:
- USB device details
- libfprint internals
- Template format
- Backend API details

### 2. Android Wrapper Layer (Kotlin/Java)

**Location**: `android-wrapper/`

**Responsibility**: 
- USB permission management
- SDK lifecycle
- Error translation
- Thread management
- HTTP client for backend API (template storage)

**Key Classes**:
- `FingerprintSDK`: Main SDK class
- `FingerprintSDK.Callback`: Result callbacks
- `FingerprintSDK.Config`: Configuration

### 3. JNI Bridge Layer (C/C++)

**Location**: `jni-bridge/`

**Responsibility**:
- Type conversion (Java ↔ C/C++)
- Error propagation
- Memory management
- Thread safety

**Key Functions**:
- `Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeEnroll`
- `Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeVerify`
- `Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeIdentify`

### 4. C++ Fingerprint Layer

**Location**: Uses existing `FingerprintCapture` from `libdigitalpersona/`

**Responsibility**:
- Direct libfprint integration
- USB device communication
- Fingerprint capture
- Matching algorithms

**Key Classes**:
- `FingerprintCapture`: Core fingerprint operations
- `FingerprintManagerAndroid`: JNI bridge wrapper

### 5. Native Libraries Layer

**Location**: Uses libraries from `uareu-android-libs`

**Responsibility**:
- USB device communication
- Fingerprint capture
- Matching algorithms

**Libraries**:
- libfprint
- libgusb
- libusb
- GLib, json-glib, etc.

## Data Flow

### Enrollment Flow

```
Android App
  ↓ enrollFingerprint(userId)
Android Wrapper (Kotlin)
  ↓ Request USB permission
  ↓ Initialize device
JNI Bridge (C/C++)
  ↓ Convert parameters (jint → int)
  ↓ Call C++ FingerprintCapture
C++ Layer
  ↓ Capture fingerprint (5 scans)
  ↓ Create template
  ↓ Return template data
JNI Bridge (C/C++)
  ↓ Convert result (byte[] → jbyteArray)
Android Wrapper (Kotlin)
  ↓ HTTP POST to backend API
  ↓ Store template in database (via backend)
  ↓ Call callback on main thread
Android App
  ↓ onEnrollSuccess(userId)
```

### Verification Flow

```
Android App
  ↓ verifyFingerprint(userId)
Android Wrapper (Kotlin)
  ↓ Check device ready
  ↓ HTTP GET to backend API
  ↓ Load template from database (via backend)
JNI Bridge (C/C++)
  ↓ Convert template (jbyteArray → byte[])
  ↓ Call C++ FingerprintCapture
C++ Layer
  ↓ Capture fingerprint
  ↓ Match with template
  ↓ Return score
JNI Bridge (C/C++)
  ↓ Convert result (int → jint)
Android Wrapper (Kotlin)
  ↓ Call callback on main thread
Android App
  ↓ onVerifySuccess(userId, score)
```

### Identification Flow

```
Android App
  ↓ identifyFingerprint()
Android Wrapper (Kotlin)
  ↓ Check device ready
  ↓ HTTP GET to backend API
  ↓ Load all templates from database (via backend)
  ↓ Create gallery
JNI Bridge (C/C++)
  ↓ Convert templates (jbyteArray[] → vector<byte[]>)
  ↓ Call C++ FingerprintCapture
C++ Layer
  ↓ Capture fingerprint
  ↓ Match against gallery
  ↓ Return matched user
JNI Bridge (C/C++)
  ↓ Convert result (int → jint)
Android Wrapper (Kotlin)
  ↓ Call callback on main thread
Android App
  ↓ onIdentifySuccess(userId, score)
```

## Communication with Backend

**Backend API** (Rust Axum server) is a **separate service** running on a server:

```
Android Wrapper (Kotlin)
  ↓ HTTP Client (OkHttp/Retrofit)
  ↓ HTTP POST/GET requests
Backend API (Rust Axum on server)
  ↓ Database operations
  ↓ Template storage/retrieval
Database (PostgreSQL/SQLite)
```

**Backend is NOT part of the SDK**. It's a separate service that:
- Stores fingerprint templates
- Manages user data
- Provides REST API endpoints

The SDK only needs to:
- Make HTTP requests to backend API
- Send/receive templates (Base64 encoded)
- Handle network errors

## Error Handling

Errors flow through all layers with proper translation:

1. **Native Layer (libfprint)**: Returns error codes and messages
2. **C++ Layer**: Wraps in exceptions or error codes
3. **JNI Bridge**: Converts to Java exceptions or error codes
4. **Android Wrapper**: Translates to user-friendly messages
5. **Application**: Receives clear error via callback

## Threading Model

- **Main Thread**: UI updates only
- **Background Thread**: All SDK operations (executor in Android wrapper)
- **USB Thread**: Dedicated thread for USB operations (in C++ layer)
- **Network Thread**: HTTP requests (OkHttp handles this)

## Memory Management

- **Java/Kotlin**: Automatic garbage collection
- **JNI**: Manual memory management, careful with global refs
- **C++**: Manual memory management (smart pointers where possible)
- **Native (libfprint)**: GLib reference counting

## Build Process

1. **C++ Layer**: Compile to `libfingerprint-capture.so` (uses existing code)
2. **JNI Bridge**: Compile to `libfingerprint-sdk-native.so` (links C++)
3. **Android Wrapper**: Package as AAR with all `.so` files

## Configuration

SDK can be configured via `FingerprintSDK.Config`:

```kotlin
val config = FingerprintSDK.Config(
    backendUrl = "http://api.example.com",  // Backend API URL
    enrollmentScans = 5,
    matchThreshold = 60,
    timeoutSeconds = 30
)
FingerprintSDK.initialize(this, config)
```

## Security Considerations

- Templates stored encrypted in database (backend responsibility)
- USB communication secured
- No sensitive data in logs
- Proper permission handling
- HTTPS for backend communication (recommended)

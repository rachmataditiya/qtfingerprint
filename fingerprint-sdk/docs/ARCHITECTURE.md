# SDK Architecture

## Overview

The Fingerprint SDK is designed with a clean layered architecture that abstracts all the complexity of USB communication, libfprint integration, and template management.

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
- Database structure

### 2. Android Wrapper Layer (Kotlin/Java)

**Location**: `android-wrapper/`

**Responsibility**: 
- USB permission management
- SDK lifecycle
- Error translation
- Thread management

**Key Classes**:
- `FingerprintSDK`: Main SDK class
- `FingerprintSDK.Callback`: Result callbacks
- `FingerprintSDK.Config`: Configuration

### 3. JNI Layer (C/C++)

**Location**: `rust-jni/jni-bridge/`

**Responsibility**:
- Type conversion (Java ↔ Rust)
- Error propagation
- Memory management
- Thread safety

**Key Functions**:
- `Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeEnroll`
- `Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeVerify`
- `Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeIdentify`

### 4. Rust Service Layer

**Location**: `rust-jni/rust-service/`

**Responsibility**:
- Fingerprint operations
- Template management
- Database operations
- Error handling

**Key Modules**:
- `fingerprint_service`: Core fingerprint operations
- `template_manager`: Template storage/retrieval
- `database`: Database operations
- `usb_manager`: USB device management

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
Android Wrapper
  ↓ Request USB permission
  ↓ Initialize device
JNI Layer
  ↓ Convert parameters
Rust Service
  ↓ Capture fingerprint (5 scans)
  ↓ Create template
  ↓ Store in database
  ↓ Return result
JNI Layer
  ↓ Convert result
Android Wrapper
  ↓ Call callback
Android App
  ↓ onEnrollSuccess(userId)
```

### Verification Flow

```
Android App
  ↓ verifyFingerprint(userId)
Android Wrapper
  ↓ Check device ready
JNI Layer
  ↓ Convert userId
Rust Service
  ↓ Load template from database
  ↓ Capture fingerprint
  ↓ Match with template
  ↓ Return result
JNI Layer
  ↓ Convert result
Android Wrapper
  ↓ Call callback
Android App
  ↓ onVerifySuccess(userId, score)
```

### Identification Flow

```
Android App
  ↓ identifyFingerprint()
Android Wrapper
  ↓ Check device ready
JNI Layer
  ↓ No parameters
Rust Service
  ↓ Load all templates from database
  ↓ Create gallery
  ↓ Capture fingerprint
  ↓ Match against gallery
  ↓ Return matched user
JNI Layer
  ↓ Convert result
Android Wrapper
  ↓ Call callback
Android App
  ↓ onIdentifySuccess(userId, score)
```

## Error Handling

Errors flow through all layers with proper translation:

1. **Native Layer**: Returns error codes and messages
2. **Rust Service**: Wraps in Result<T, Error>
3. **JNI Layer**: Converts to Java exceptions or error codes
4. **Android Wrapper**: Translates to user-friendly messages
5. **Application**: Receives clear error via callback

## Threading Model

- **Main Thread**: UI updates only
- **Background Thread**: All SDK operations
- **USB Thread**: Dedicated thread for USB operations
- **Database Thread**: Async database operations

## Memory Management

- **Java/Kotlin**: Automatic garbage collection
- **JNI**: Manual memory management, careful with global refs
- **Rust**: Ownership-based memory management
- **Native**: GLib reference counting

## Configuration

SDK can be configured via `FingerprintSDK.Config`:

```kotlin
val config = FingerprintSDK.Config(
    backendUrl = "http://api.example.com",
    enrollmentScans = 5,
    matchThreshold = 60,
    timeoutSeconds = 30
)
FingerprintSDK.initialize(this, config)
```

## Security Considerations

- Templates stored encrypted in database
- USB communication secured
- No sensitive data in logs
- Proper permission handling


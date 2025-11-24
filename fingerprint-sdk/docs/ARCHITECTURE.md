# Arkana Fingerprint SDK - Architecture

## System Architecture

```
App Android
   │ Kotlin API
   ▼
Arkana Fingerprint SDK
   ├─ Capture Layer (JNI ↔ libfprint)
   ├─ Matching Engine (local)
   ├─ Backend API (Rust)
   └─ Secure Cache (AES + Keystore)
```

## Module Structure

```
com.arkana.fingerprint.sdk/
  ├── FingerprintSdk.kt          # Main API
  ├── config/                     # Configuration
  │   └── FingerprintSdkConfig.kt
  ├── capture/                    # Capture operations
  │   ├── CaptureManager.kt
  │   └── native/
  │       └── LibfprintNative.kt  # JNI bridge
  ├── matching/                   # Matching engine
  │   └── MatchingEngine.kt
  ├── backend/                    # Backend API client
  │   └── BackendClient.kt
  ├── cache/                      # Secure cache
  │   └── SecureCache.kt
  ├── model/                      # Data models
  │   ├── Finger.kt
  │   ├── CaptureResult.kt
  │   ├── EnrollResult.kt
  │   ├── VerifyResult.kt
  │   └── IdentifyResult.kt
  └── util/                       # Utilities
      └── FingerError.kt
```

## Layer Breakdown

### 1. Kotlin API Layer

**FingerprintSdk.kt** - Main entry point:
- `init(config, context)` - Initialize SDK
- `enroll(userId, finger)` - Enroll fingerprint
- `verify(userId)` - Verify 1:1
- `identify(scope)` - Identify 1:N

### 2. Capture Layer

**CaptureManager.kt**:
- USB device management
- Fingerprint capture coordination

**LibfprintNative.kt** (JNI bridge):
- Calls native `arkana_fprint.cpp`
- Type conversion Java ↔ C++

**arkana_fprint.cpp** (Native):
- Uses existing `FingerprintCapture` from `libdigitalpersona/`
- Direct libfprint integration

### 3. Matching Engine

**MatchingEngine.kt**:
- Template matching using libfprint
- Score calculation (0.0-1.0)

### 4. Backend Client

**BackendClient.kt**:
- HTTP client for Rust Axum API
- Template storage/retrieval
- Authentication logging

### 5. Secure Cache

**SecureCache.kt**:
- AES-256-GCM encryption
- Android Keystore key management
- Binary encrypted format

## Data Flow

### Enrollment Flow

```
FingerprintSdk.enroll()
  ↓
CaptureManager.captureOnce() × 5
  ↓
LibfprintNative.capture()
  ↓
arkana_fprint.cpp → FingerprintCapture
  ↓
libfprint → USB device
  ↓
MatchingEngine.createTemplate()
  ↓
BackendClient.storeTemplate()
  ↓
SecureCache.store() (encrypted)
```

### Verification Flow

```
FingerprintSdk.verify()
  ↓
SecureCache.load() or BackendClient.loadTemplate()
  ↓
CaptureManager.captureOnce()
  ↓
MatchingEngine.match()
  ↓
BackendClient.logAuth()
```

## Native Libraries

SDK includes:
- `libarkana_fprint.so` - Native JNI bridge
- `libfprint-2.so` - Fingerprint library
- `libgusb.so`, `libusb-1.0.so` - USB communication
- `libglib-2.0.so`, `libgobject-2.0.so`, `libgio-2.0.so` - GLib
- `libjson-glib-1.0.so` - JSON parsing
- `libffi.so` - Foreign function interface
- `libcrypto.so`, `libssl.so` - OpenSSL

## Threading Model

- **Main Thread**: UI updates
- **IO Dispatcher**: Capture, backend API calls
- **Default Dispatcher**: Matching operations

## Security

- Templates encrypted with AES-256-GCM
- Keys stored in Android Keystore
- No plaintext templates
- HTTPS recommended for backend


# Arkana Fingerprint SDK

SDK untuk operasi fingerprint pada Android menggunakan DigitalPersona U.are.U 4000.

## Overview

SDK ini menyediakan API yang clean dan simple untuk:
- **Capture**: Menangkap fingerprint dari hardware
- **Enroll**: Mendaftarkan fingerprint untuk user
- **Verify**: Verifikasi 1:1 (satu user, satu template)
- **Identify**: Identifikasi 1:N (cari user dari banyak template)

## Features

✅ **Simple API** - Hanya 3 method utama: enroll, verify, identify  
✅ **Automatic USB Management** - Handles permissions dan device initialization  
✅ **Secure Cache** - Template di-cache dengan AES-256-GCM + Android Keystore  
✅ **Backend Integration** - Komunikasi dengan Rust API untuk template storage  
✅ **Local Matching** - Matching dilakukan di device menggunakan libfprint  
✅ **Coroutine Support** - Menggunakan Kotlin Coroutines untuk async operations  
✅ **Error Handling** - Error handling yang comprehensive dengan `FingerError` enum  

## Quick Start

### 1. Initialize SDK

```kotlin
import com.arkana.fingerprint.sdk.FingerprintSdk

val config = FingerprintSdkConfig(
    backendUrl = "http://api.example.com",
    enrollmentScans = 5,
    matchThreshold = 0.6f,
    enableCache = true
)

FingerprintSdk.init(config, context)
```

### 2. Enroll Fingerprint

```kotlin
val result = FingerprintSdk.enroll(
    userId = 123,
    finger = Finger.LEFT_INDEX
)

when (result) {
    is EnrollResult.Success -> {
        println("Enrollment successful")
    }
    is EnrollResult.Error -> {
        println("Error: ${result.error}")
    }
}
```

### 3. Verify Fingerprint

```kotlin
val result = FingerprintSdk.verify(userId = 123)

when (result) {
    is VerifyResult.Success -> {
        println("Verified! Score: ${result.score}")
    }
    is VerifyResult.Error -> {
        println("Error: ${result.error}")
    }
}
```

### 4. Identify User

```kotlin
val result = FingerprintSdk.identify(scope = "branch_001")

when (result) {
    is IdentifyResult.Success -> {
        println("User identified: ${result.userName} (${result.userId})")
        println("Finger: ${result.finger}, Score: ${result.score}")
        if (result.userEmail != null) {
            println("Email: ${result.userEmail}")
        }
    }
    is IdentifyResult.Error -> {
        println("Error: ${result.error}")
    }
}
```

## Architecture

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
  ├── capture/                    # Capture operations
  ├── matching/                   # Matching engine
  ├── backend/                    # Backend API client
  ├── cache/                      # Secure cache
  ├── model/                      # Data models
  └── util/                       # Utilities
```

## Requirements

- Android API level 28 (Android 9.0) or higher
- USB Host support on device
- DigitalPersona U.are.U 4000 fingerprint reader
- Backend API running (for template storage)

## Dependencies

- libfprint (from `uareu-android-libs`)
- libgusb, libusb
- GLib, json-glib, libffi, OpenSSL

## Documentation

- [Integration Guide](docs/INTEGRATION.md) - How to integrate SDK into your project
- [Usage Guide](docs/USAGE.md) - Complete usage guide with examples
- [API Reference](docs/API_REFERENCE.md) - Complete API documentation
- [Architecture](docs/ARCHITECTURE.md) - System architecture
- [Backend API](docs/BACKEND_API.md) - Backend API specification
- [Troubleshooting](docs/TROUBLESHOOTING.md) - Common issues and solutions
- [Examples](examples/) - Code examples

## License

Proprietary - Arkana


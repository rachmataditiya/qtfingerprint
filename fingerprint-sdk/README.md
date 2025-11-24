# Fingerprint SDK for Android

A clean, high-level SDK for fingerprint operations on Android devices using DigitalPersona U.are.U 4000.

## Overview

This SDK provides a simple, clean API for Android applications to perform fingerprint operations without needing to understand the underlying complexity of libfprint, libusb, or USB Host API integration.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│           Android Application (Kotlin/Java)            │
│  enrollFingerprint(userId)                             │
│  verifyFingerprint(userId)                             │
│  identifyFingerprint()                                 │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│         Android Wrapper (Kotlin/Java)                  │
│  FingerprintSDK class                                   │
│  - Manages USB permissions                              │
│  - Handles lifecycle                                    │
│  - Provides clean API                                   │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│              JNI Layer (C/C++)                          │
│  Native methods bridge                                  │
│  - Error handling                                       │
│  - Type conversion                                      │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│            Rust Service Layer                           │
│  - Fingerprint operations                               │
│  - Template management                                  │
│  - Database integration                                 │
└───────────────────┬─────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────────────────┐
│         libfprint + Android Libraries                   │
│  - USB device communication                             │
│  - Fingerprint capture                                  │
│  - Matching algorithms                                  │
└─────────────────────────────────────────────────────────┘
```

## Features

✅ **Simple API** - Just 3 methods: enroll, verify, identify  
✅ **Automatic USB Management** - Handles permissions and device initialization  
✅ **Error Handling** - Clear error messages and status codes  
✅ **Thread-Safe** - Safe to call from any thread  
✅ **Lifecycle Aware** - Properly manages resources  
✅ **Well Documented** - Complete API reference and examples  

## Quick Start

### 1. Add Dependency

```gradle
dependencies {
    implementation 'com.arkana:fingerprint-sdk:1.0.0'
}
```

### 2. Initialize SDK

```kotlin
import com.arkana.fingerprintsdk.FingerprintSDK

class MainActivity : AppCompatActivity() {
    private lateinit var fingerprintSDK: FingerprintSDK
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Initialize SDK
        fingerprintSDK = FingerprintSDK.initialize(this)
        
        // Set callback for results
        fingerprintSDK.setCallback(object : FingerprintSDK.Callback {
            override fun onEnrollSuccess(userId: Int) {
                Log.d(TAG, "Enrollment successful for user $userId")
            }
            
            override fun onEnrollError(userId: Int, error: String) {
                Log.e(TAG, "Enrollment failed: $error")
            }
            
            override fun onVerifySuccess(userId: Int, score: Int) {
                Log.d(TAG, "Verification successful: score=$score")
            }
            
            override fun onVerifyError(userId: Int, error: String) {
                Log.e(TAG, "Verification failed: $error")
            }
            
            override fun onIdentifySuccess(userId: Int, score: Int) {
                Log.d(TAG, "User identified: userId=$userId, score=$score")
            }
            
            override fun onIdentifyError(error: String) {
                Log.e(TAG, "Identification failed: $error")
            }
        })
    }
}
```

### 3. Use SDK

```kotlin
// Enroll fingerprint for user
fingerprintSDK.enrollFingerprint(userId = 123)

// Verify fingerprint
fingerprintSDK.verifyFingerprint(userId = 123)

// Identify user (1:N matching)
fingerprintSDK.identifyFingerprint()
```

## API Reference

See [API_REFERENCE.md](docs/API_REFERENCE.md) for complete API documentation.

## Examples

See [examples/](examples/) directory for complete working examples.

## Requirements

- Android API level 28 (Android 9.0) or higher
- USB Host support on device
- DigitalPersona U.are.U 4000 fingerprint reader
- Backend API running (for template storage)

## License

See [LICENSE.md](LICENSE.md) for license information.


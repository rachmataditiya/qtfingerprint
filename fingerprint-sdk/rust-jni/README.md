# JNI Bridge

This directory contains the JNI bridge that connects the Android Kotlin/Java layer with the C++ fingerprint operations.

## Structure

```
jni-bridge/
├── jni_bridge.cpp      # JNI native methods
├── jni_bridge.h        # Header file
└── CMakeLists.txt      # Build configuration
```

## Architecture

The JNI bridge:
- Receives calls from Kotlin/Java (`FingerprintSDK.kt`)
- Calls C++ `FingerprintCapture` (from `libdigitalpersona/`)
- Converts types between Java and C++
- Handles errors and exceptions

## Functions

- `Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeEnroll`
- `Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeVerify`
- `Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeIdentify`

## Building

See [BUILD.md](../docs/BUILD.md) for build instructions.

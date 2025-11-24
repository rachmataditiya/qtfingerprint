# Android Wrapper Layer

This directory contains the Android Kotlin/Java wrapper that provides the clean API for applications.

## Structure

```
android-wrapper/
├── src/main/java/com/arkana/fingerprintsdk/
│   ├── FingerprintSDK.kt          # Main SDK class
│   ├── FingerprintSDKCallback.kt # Callback interface
│   ├── FingerprintSDKConfig.kt    # Configuration class
│   ├── FingerprintSDKStatus.kt    # Status enum
│   └── exceptions/
│       └── SDKException.kt       # SDK exceptions
├── src/main/AndroidManifest.xml
└── build.gradle.kts
```

## Features

- **USB Permission Management**: Automatic USB permission requests
- **Lifecycle Management**: Proper initialization and cleanup
- **Error Translation**: User-friendly error messages
- **Thread Safety**: Safe to call from any thread
- **Callback-based**: Asynchronous operations with callbacks

## Usage

See main [README.md](../README.md) for usage examples.

## Integration

The wrapper is built as an Android library (AAR) that can be included in any Android project.


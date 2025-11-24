# Rust JNI Service Layer

This directory contains the Rust service layer that bridges between JNI and fingerprint operations.

## Structure

```
rust-jni/
├── rust-service/          # Rust service implementation
│   ├── src/
│   │   ├── lib.rs         # Main library
│   │   ├── fingerprint.rs # Fingerprint operations
│   │   ├── template.rs    # Template management
│   │   ├── database.rs    # Database operations
│   │   └── usb.rs         # USB device management
│   └── Cargo.toml
├── jni-bridge/            # JNI C/C++ bridge
│   ├── jni_bridge.cpp     # JNI native methods
│   ├── jni_bridge.h       # Header file
│   └── CMakeLists.txt     # Build configuration
└── README.md              # This file
```

## Architecture

The Rust service layer provides:

1. **Fingerprint Operations**: Capture, enroll, verify, identify
2. **Template Management**: Store, load, serialize templates
3. **Database Integration**: CRUD operations for templates
4. **USB Management**: Device initialization and communication
5. **Error Handling**: Comprehensive error types and messages

## Building

See [BUILD.md](../docs/BUILD.md) for build instructions.

## API

The Rust service exposes functions that are called via JNI:

- `enroll_fingerprint(user_id: i32) -> Result<(), Error>`
- `verify_fingerprint(user_id: i32) -> Result<bool, Error>`
- `identify_fingerprint() -> Result<Option<i32>, Error>`

These functions are wrapped by JNI layer and exposed to Android.


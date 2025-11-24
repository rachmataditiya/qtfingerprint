# Integration Guide

Panduan lengkap untuk mengintegrasikan Arkana Fingerprint SDK ke project Android Anda.

## Prerequisites

- Android Studio Arctic Fox atau lebih baru
- Android SDK dengan API level 28+
- Gradle 8.0+
- Kotlin 1.9+

## Installation

### Option 1: AAR File (Recommended)

1. **Copy AAR file** ke project Anda:
   ```bash
   cp fingerprint-sdk-debug.aar your-project/app/libs/
   ```

2. **Tambahkan ke `build.gradle.kts`**:
   ```kotlin
   dependencies {
       implementation(files("libs/fingerprint-sdk-debug.aar"))
   }
   ```

### Option 2: Module Dependency

1. **Copy SDK folder** ke project Anda:
   ```bash
   cp -r fingerprint-sdk/android-wrapper your-project/fingerprint-sdk
   ```

2. **Tambahkan ke `settings.gradle.kts`**:
   ```kotlin
   include(":fingerprint-sdk")
   project(":fingerprint-sdk").projectDir = file("fingerprint-sdk")
   ```

3. **Tambahkan dependency** di `app/build.gradle.kts`:
   ```kotlin
   dependencies {
       implementation(project(":fingerprint-sdk"))
   }
   ```

## Dependencies

SDK membutuhkan dependencies berikut (sudah included dalam AAR):

- Kotlin Coroutines
- OkHttp
- Kotlinx Serialization
- AndroidX Core & AppCompat

## Permissions

Tambahkan ke `AndroidManifest.xml`:

```xml
<!-- USB permissions -->
<uses-feature
    android:name="android.hardware.usb.host"
    android:required="true" />

<uses-permission android:name="android.permission.USB_PERMISSION" />

<!-- Network permission for backend API -->
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
```

## USB Device Filter

Tambahkan `res/xml/device_filter.xml`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <!-- DigitalPersona U.are.U 4000 -->
    <usb-device vendor-id="1466" product-id="10" />
    <usb-device vendor-id="1466" product-id="52" />
</resources>
```

Dan tambahkan ke `AndroidManifest.xml`:

```xml
<activity ...>
    <meta-data
        android:name="android.hardware.usb.action.USB_DEVICE_ATTACHED"
        android:resource="@xml/device_filter" />
</activity>
```

## Quick Start

### 1. Initialize SDK

```kotlin
import com.arkana.fingerprint.sdk.FingerprintSdk
import com.arkana.fingerprint.sdk.config.FingerprintSdkConfig

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Initialize SDK
        val config = FingerprintSdkConfig(
            backendUrl = "http://your-backend-api.com",
            enrollmentScans = 5,
            matchThreshold = 0.6f,
            enableCache = true
        )
        
        FingerprintSdk.init(config, this)
    }
}
```

### 2. Enroll Fingerprint

```kotlin
lifecycleScope.launch {
    val result = FingerprintSdk.enroll(
        userId = 123,
        finger = Finger.LEFT_INDEX
    )
    
    when (result) {
        is EnrollResult.Success -> {
            Toast.makeText(this@MainActivity, 
                "Enrollment successful!", Toast.LENGTH_SHORT).show()
        }
        is EnrollResult.Error -> {
            Toast.makeText(this@MainActivity, 
                "Error: ${result.error}", Toast.LENGTH_LONG).show()
        }
    }
}
```

### 3. Verify Fingerprint

```kotlin
lifecycleScope.launch {
    val result = FingerprintSdk.verify(userId = 123)
    
    when (result) {
        is VerifyResult.Success -> {
            Toast.makeText(this@MainActivity, 
                "Verified! Score: ${result.score}", Toast.LENGTH_SHORT).show()
        }
        is VerifyResult.Error -> {
            Toast.makeText(this@MainActivity, 
                "Verification failed", Toast.LENGTH_LONG).show()
        }
    }
}
```

### 4. Identify User

```kotlin
lifecycleScope.launch {
    val result = FingerprintSdk.identify(scope = "branch_001")
    
    when (result) {
        is IdentifyResult.Success -> {
            Toast.makeText(this@MainActivity, 
                "User ${result.userName} (${result.finger}) identified! Score: ${result.score}", 
                Toast.LENGTH_SHORT).show()
        }
        is IdentifyResult.Error -> {
            Toast.makeText(this@MainActivity, 
                "No match found", Toast.LENGTH_LONG).show()
        }
    }
}
```

## Configuration

### FingerprintSdkConfig

```kotlin
data class FingerprintSdkConfig(
    val backendUrl: String,              // Backend API URL (required)
    val enrollmentScans: Int = 5,        // Number of scans for enrollment
    val matchThreshold: Float = 0.6f,    // Match threshold (0.0-1.0)
    val timeoutMs: Long = 30000,         // Operation timeout
    val enableCache: Boolean = true      // Enable secure cache
)
```

## Error Handling

SDK menggunakan `FingerError` enum untuk error handling:

```kotlin
enum class FingerError {
    SDK_NOT_INITIALIZED,
    DEVICE_NOT_FOUND,
    USB_PERMISSION_DENIED,
    CAPTURE_FAILED,
    VERIFICATION_FAILED,
    IDENTIFICATION_FAILED,
    NETWORK_ERROR,
    BACKEND_ERROR,
    // ... more errors
}
```

## Backend API Requirements

SDK membutuhkan backend API dengan endpoints berikut:

- `POST /users/{userId}/fingerprint` - Store template
- `GET /users/{userId}/fingerprint` - Load template
- `GET /templates?scope={scope}` - Load all templates
- `POST /log_auth` - Log authentication events

Lihat [BACKEND_API.md](BACKEND_API.md) untuk detail API specification.

## Troubleshooting

### Device Not Found

- Pastikan USB device terhubung
- Cek USB permission sudah granted
- Restart app setelah connect device

### Network Error

- Cek backend API URL benar
- Pastikan backend API running
- Cek network connectivity

### Build Errors

- Pastikan minSdk = 28
- Pastikan semua dependencies ter-resolve
- Clean dan rebuild project

Lihat [TROUBLESHOOTING.md](TROUBLESHOOTING.md) untuk lebih detail.

## Examples

Lihat folder `examples/` untuk contoh penggunaan lengkap.

## Support

Untuk issues atau questions, buka issue di repository.


# Usage Guide

Panduan penggunaan Arkana Fingerprint SDK dalam aplikasi Android.

## Table of Contents

1. [Initialization](#initialization)
2. [Enrollment](#enrollment)
3. [Verification](#verification)
4. [Identification](#identification)
5. [Error Handling](#error-handling)
6. [Best Practices](#best-practices)

## Initialization

### Basic Initialization

```kotlin
import com.arkana.fingerprint.sdk.FingerprintSdk
import com.arkana.fingerprint.sdk.config.FingerprintSdkConfig

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        val config = FingerprintSdkConfig(
            backendUrl = "http://api.example.com"
        )
        
        FingerprintSdk.init(config, this)
    }
}
```

### Custom Configuration

```kotlin
val config = FingerprintSdkConfig(
    backendUrl = "https://api.production.com",
    enrollmentScans = 3,           // Fewer scans for faster enrollment
    matchThreshold = 0.7f,        // Higher threshold (stricter matching)
    timeoutMs = 60000,            // Longer timeout
    enableCache = true             // Enable secure cache
)

FingerprintSdk.init(config, this)
```

## Enrollment

### Basic Enrollment

```kotlin
lifecycleScope.launch {
    val result = FingerprintSdk.enroll(
        userId = 123,
        finger = Finger.LEFT_INDEX
    )
    
    when (result) {
        is EnrollResult.Success -> {
            // Enrollment successful
            Log.d(TAG, "User ${result.userId} enrolled")
        }
        is EnrollResult.Error -> {
            // Handle error
            Log.e(TAG, "Enrollment failed: ${result.error}")
        }
    }
}
```

### Enrollment with Progress

```kotlin
class EnrollmentActivity : AppCompatActivity() {
    private var currentScan = 0
    private val totalScans = 5
    
    fun startEnrollment(userId: Int) {
        lifecycleScope.launch {
            val result = FingerprintSdk.enroll(userId, Finger.LEFT_INDEX)
            
            when (result) {
                is EnrollResult.Success -> {
                    progressBar.progress = 100
                    statusText.text = "Enrollment complete!"
                }
                is EnrollResult.Error -> {
                    statusText.text = "Error: ${result.error}"
                }
            }
        }
    }
}
```

## Verification

### Basic Verification

```kotlin
lifecycleScope.launch {
    val result = FingerprintSdk.verify(userId = 123)
    
    when (result) {
        is VerifyResult.Success -> {
            if (result.score >= 0.7f) {
                // High confidence match
                showSuccess("Verified! Score: ${result.score}")
            } else {
                // Low confidence
                showWarning("Verified but low score: ${result.score}")
            }
        }
        is VerifyResult.Error -> {
            showError("Verification failed: ${result.error}")
        }
    }
}
```

### Verification with Retry

```kotlin
suspend fun verifyWithRetry(userId: Int, maxRetries: Int = 3): VerifyResult {
    var retryCount = 0
    
    while (retryCount < maxRetries) {
        val result = FingerprintSdk.verify(userId)
        
        when (result) {
            is VerifyResult.Success -> return result
            is VerifyResult.Error -> {
                retryCount++
                if (retryCount < maxRetries) {
                    delay(1000) // Wait 1 second before retry
                }
            }
        }
    }
    
    return VerifyResult.Error(FingerError.VERIFICATION_FAILED)
}
```

## Identification

### Basic Identification

```kotlin
lifecycleScope.launch {
    val result = FingerprintSdk.identify(scope = "branch_001")
    
    when (result) {
        is IdentifyResult.Success -> {
            // Result includes user name, email, finger, and score
            showUserInfo(
                name = result.userName,
                email = result.userEmail,
                userId = result.userId,
                finger = result.finger,
                score = result.score
            )
        }
        is IdentifyResult.Error -> {
            showError("No user identified: ${result.error}")
        }
    }
}
```

### Identification with Scope

```kotlin
// Identify within specific branch
val result = FingerprintSdk.identify(scope = "branch_001")

// Identify from all users
val result = FingerprintSdk.identify(scope = null)
```

## Error Handling

### Error Types

```kotlin
enum class FingerError {
    // Initialization
    SDK_NOT_INITIALIZED,
    DEVICE_NOT_FOUND,
    DEVICE_INIT_FAILED,
    
    // USB
    USB_PERMISSION_DENIED,
    USB_DEVICE_NOT_FOUND,
    
    // Capture
    CAPTURE_TIMEOUT,
    CAPTURE_FAILED,
    
    // Matching
    VERIFICATION_FAILED,
    IDENTIFICATION_FAILED,
    NO_TEMPLATES,
    
    // Backend
    NETWORK_ERROR,
    BACKEND_ERROR,
    TEMPLATE_NOT_FOUND,
    
    // Unknown
    UNKNOWN_ERROR
}
```

### Error Handling Example

```kotlin
when (result) {
    is EnrollResult.Error -> {
        when (result.error) {
            FingerError.USB_PERMISSION_DENIED -> {
                // Request USB permission
                requestUsbPermission()
            }
            FingerError.DEVICE_NOT_FOUND -> {
                // Show message: Connect device
                showMessage("Please connect fingerprint reader")
            }
            FingerError.NETWORK_ERROR -> {
                // Check network connection
                checkNetworkConnection()
            }
            else -> {
                // Generic error
                showError("Error: ${result.error}")
            }
        }
    }
    // ...
}
```

## Best Practices

### 1. Initialize Once

```kotlin
// ✅ Good: Initialize in Application class
class MyApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        FingerprintSdk.init(config, this)
    }
}

// ❌ Bad: Initialize multiple times
class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        FingerprintSdk.init(config, this) // Don't do this
    }
}
```

### 2. Use Coroutines

```kotlin
// ✅ Good: Use lifecycleScope
lifecycleScope.launch {
    val result = FingerprintSdk.verify(userId)
    // Handle result
}

// ❌ Bad: Blocking call
val result = FingerprintSdk.verify(userId) // Don't do this
```

### 3. Handle Errors Properly

```kotlin
// ✅ Good: Handle all error cases
when (result) {
    is VerifyResult.Success -> { /* ... */ }
    is VerifyResult.Error -> {
        when (result.error) {
            FingerError.DEVICE_NOT_FOUND -> { /* ... */ }
            FingerError.CAPTURE_FAILED -> { /* ... */ }
            else -> { /* ... */ }
        }
    }
}
```

### 4. Check Device Before Operations

```kotlin
// Check if device is ready (optional, SDK handles this)
if (usbDevice != null) {
    lifecycleScope.launch {
        val result = FingerprintSdk.verify(userId)
        // ...
    }
} else {
    showMessage("Please connect fingerprint reader")
}
```

### 5. Use Appropriate Scope

```kotlin
// For identification, use scope to limit search
val result = FingerprintSdk.identify(scope = "branch_001")

// This is faster than searching all users
```

## Complete Example

```kotlin
class FingerprintActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Initialize SDK
        val config = FingerprintSdkConfig(
            backendUrl = "http://api.example.com",
            enrollmentScans = 5,
            matchThreshold = 0.6f
        )
        FingerprintSdk.init(config, this)
        
        // Setup UI
        setupUI()
    }
    
    private fun setupUI() {
        enrollButton.setOnClickListener {
            lifecycleScope.launch {
                enrollUser(123)
            }
        }
        
        verifyButton.setOnClickListener {
            lifecycleScope.launch {
                verifyUser(123)
            }
        }
        
        identifyButton.setOnClickListener {
            lifecycleScope.launch {
                identifyUser()
            }
        }
    }
    
    private suspend fun enrollUser(userId: Int) {
        showProgress("Enrolling fingerprint...")
        
        val result = FingerprintSdk.enroll(userId, Finger.LEFT_INDEX)
        
        when (result) {
            is EnrollResult.Success -> {
                hideProgress()
                showSuccess("Enrollment successful!")
            }
            is EnrollResult.Error -> {
                hideProgress()
                showError("Enrollment failed: ${result.error}")
            }
        }
    }
    
    private suspend fun verifyUser(userId: Int) {
        showProgress("Verifying fingerprint...")
        
        val result = FingerprintSdk.verify(userId)
        
        when (result) {
            is VerifyResult.Success -> {
                hideProgress()
                if (result.score >= 0.7f) {
                    showSuccess("Verified! Score: ${result.score}")
                } else {
                    showWarning("Low score: ${result.score}")
                }
            }
            is VerifyResult.Error -> {
                hideProgress()
                showError("Verification failed: ${result.error}")
            }
        }
    }
    
    private suspend fun identifyUser() {
        showProgress("Identifying user...")
        
        val result = FingerprintSdk.identify(scope = "branch_001")
        
        when (result) {
            is IdentifyResult.Success -> {
                hideProgress()
                showSuccess(
                    "User identified: ${result.userName} (${result.finger})\n" +
                    "Score: ${result.score}"
                )
            }
            is IdentifyResult.Error -> {
                hideProgress()
                showError("No match found: ${result.error}")
            }
        }
    }
}
```

## See Also

- [INTEGRATION.md](INTEGRATION.md) - Integration guide
- [API_REFERENCE.md](API_REFERENCE.md) - Complete API reference
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Troubleshooting guide


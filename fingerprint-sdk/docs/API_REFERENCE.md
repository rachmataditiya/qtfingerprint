# API Reference

Complete API reference for Arkana Fingerprint SDK.

## FingerprintSdk

Main SDK class for fingerprint operations.

### Initialization

#### `init(config: FingerprintSdkConfig, context: Context)`

Initialize the SDK.

**Parameters:**
- `config`: SDK configuration
- `context`: Android Context

**Throws:**
- `IllegalStateException` if already initialized

**Example:**
```kotlin
val config = FingerprintSdkConfig(
    backendUrl = "http://api.example.com"
)
FingerprintSdk.init(config, context)
```

### Capture

#### `captureOnce(timeoutMs: Long = 30000): CaptureResult`

Capture fingerprint once.

**Parameters:**
- `timeoutMs`: Timeout in milliseconds (default: 30000)

**Returns:**
- `CaptureResult.Success(template: ByteArray)` on success
- `CaptureResult.Error(error: FingerError)` on failure

**Example:**
```kotlin
val result = FingerprintSdk.captureOnce(30000)
when (result) {
    is CaptureResult.Success -> {
        // Use result.template
    }
    is CaptureResult.Error -> {
        // Handle error
    }
}
```

### Enrollment

#### `enroll(userId: Int, finger: Finger = Finger.UNKNOWN): EnrollResult`

Enroll fingerprint for a user.

**Parameters:**
- `userId`: User ID to enroll
- `finger`: Finger type (default: UNKNOWN)

**Returns:**
- `EnrollResult.Success(userId: Int)` on success
- `EnrollResult.Error(error: FingerError)` on failure

**Process:**
1. Captures multiple scans (based on config)
2. Creates template from scans
3. Stores template in backend
4. Caches template locally (if enabled)

**Example:**
```kotlin
val result = FingerprintSdk.enroll(123, Finger.LEFT_INDEX)
```

### Verification

#### `verify(userId: Int, finger: Finger? = null): VerifyResult`

Verify fingerprint for a user (1:1 matching).

**Parameters:**
- `userId`: User ID to verify
- `finger`: Optional finger to verify (if null, uses most recent finger)

**Returns:**
- `VerifyResult.Success(userId: Int, score: Float)` on match
- `VerifyResult.Error(error: FingerError)` on failure

**Process:**
1. Loads template from cache or backend (for specific finger if provided)
2. Captures current fingerprint
3. Matches against stored template
4. Returns score (0.0-1.0)
5. Logs authentication event

**Example:**
```kotlin
// Verify most recent finger
val result = FingerprintSdk.verify(userId = 123)

// Verify specific finger
val result = FingerprintSdk.verify(userId = 123, finger = Finger.LEFT_INDEX)

when (result) {
    is VerifyResult.Success -> {
        println("Score: ${result.score}")
    }
    is VerifyResult.Error -> {
        println("Error: ${result.error}")
    }
}
```

### Identification

#### `identify(scope: String? = null): IdentifyResult`

Identify user from fingerprint (1:N matching). Searches all fingers for all users.

**Parameters:**
- `scope`: Optional scope filter (e.g., branch ID)

**Returns:**
- `IdentifyResult.Success(userId: Int, userName: String, userEmail: String?, finger: String, score: Float)` on match
- `IdentifyResult.Error(error: FingerError)` on failure

**Process:**
1. Loads all templates from backend (all fingers for all users, optionally filtered by scope)
2. Captures current fingerprint
3. Matches against all templates
4. Returns matched user ID, name, email, finger, and score
5. Logs authentication event

**Note:** Identification searches all fingers for all users. If a user has multiple fingers enrolled, all will be checked.

**Example:**
```kotlin
val result = FingerprintSdk.identify(scope = "branch_001")

when (result) {
    is IdentifyResult.Success -> {
        println("User: ${result.userName} (ID: ${result.userId})")
        println("Finger: ${result.finger}")
        println("Score: ${result.score}")
        if (result.userEmail != null) {
            println("Email: ${result.userEmail}")
        }
    }
    is IdentifyResult.Error -> {
        println("Error: ${result.error}")
    }
}
```

## Configuration

### FingerprintSdkConfig

```kotlin
data class FingerprintSdkConfig(
    val backendUrl: String,              // Required: Backend API URL
    val enrollmentScans: Int = 5,        // Number of scans for enrollment
    val matchThreshold: Float = 0.6f,    // Match threshold (0.0-1.0)
    val timeoutMs: Long = 30000,          // Operation timeout
    val enableCache: Boolean = true       // Enable secure cache
)
```

**Properties:**
- `backendUrl`: Backend API base URL (required)
- `enrollmentScans`: Number of fingerprint scans for enrollment (default: 5)
- `matchThreshold`: Minimum score for successful match (0.0-1.0, default: 0.6)
- `timeoutMs`: Operation timeout in milliseconds (default: 30000)
- `enableCache`: Enable secure cache for templates (default: true)

## Models

### Finger

```kotlin
enum class Finger {
    UNKNOWN,
    LEFT_THUMB,
    LEFT_INDEX,
    LEFT_MIDDLE,
    LEFT_RING,
    LEFT_PINKY,
    RIGHT_THUMB,
    RIGHT_INDEX,
    RIGHT_MIDDLE,
    RIGHT_RING,
    RIGHT_PINKY
}
```

### Results

#### CaptureResult

```kotlin
sealed class CaptureResult {
    data class Success(val template: ByteArray) : CaptureResult()
    data class Error(val error: FingerError) : CaptureResult()
}
```

#### EnrollResult

```kotlin
sealed class EnrollResult {
    data class Success(val userId: Int) : EnrollResult()
    data class Error(val error: FingerError) : EnrollResult()
}
```

#### VerifyResult

```kotlin
sealed class VerifyResult {
    data class Success(val userId: Int, val score: Float) : VerifyResult()
    data class Error(val error: FingerError) : VerifyResult()
}
```

#### IdentifyResult

```kotlin
sealed class IdentifyResult {
    data class Success(val userId: Int, val score: Float) : IdentifyResult()
    data class Error(val error: FingerError) : IdentifyResult()
}
```

## Error Handling

### FingerError

```kotlin
enum class FingerError {
    // Initialization
    SDK_NOT_INITIALIZED,
    DEVICE_NOT_FOUND,
    DEVICE_INIT_FAILED,
    
    // USB
    USB_PERMISSION_DENIED,
    USB_DEVICE_NOT_FOUND,
    USB_ERROR,
    
    // Capture
    CAPTURE_TIMEOUT,
    CAPTURE_FAILED,
    CAPTURE_CANCELLED,
    
    // Matching
    MATCHING_FAILED,
    VERIFICATION_FAILED,
    IDENTIFICATION_FAILED,
    NO_TEMPLATES,
    
    // Backend
    BACKEND_ERROR,
    NETWORK_ERROR,
    TEMPLATE_NOT_FOUND,
    
    // Cache
    CACHE_ERROR,
    DECRYPTION_FAILED,
    
    // Unknown
    UNKNOWN_ERROR
}
```

## Threading

All SDK operations are **suspend functions** and should be called from coroutines:

```kotlin
lifecycleScope.launch {
    val result = FingerprintSdk.verify(userId)
    // Handle result
}
```

Operations run on `Dispatchers.IO` internally.

## See Also

- [USAGE.md](USAGE.md) - Usage guide with examples
- [INTEGRATION.md](INTEGRATION.md) - Integration guide
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Troubleshooting


# API Reference

Complete API documentation for the Fingerprint SDK.

## FingerprintSDK

Main SDK class for fingerprint operations.

### Initialization

#### `initialize(context: Context): FingerprintSDK`

Initialize the SDK with default configuration.

**Parameters:**
- `context`: Android Context (Activity or Application)

**Returns:**
- `FingerprintSDK` instance

**Throws:**
- `SDKException` if initialization fails

**Example:**
```kotlin
val sdk = FingerprintSDK.initialize(this)
```

#### `initialize(context: Context, config: Config): FingerprintSDK`

Initialize the SDK with custom configuration.

**Parameters:**
- `context`: Android Context
- `config`: SDK configuration

**Returns:**
- `FingerprintSDK` instance

**Example:**
```kotlin
val config = FingerprintSDK.Config(
    backendUrl = "http://api.example.com",
    enrollmentScans = 5,
    matchThreshold = 60
)
val sdk = FingerprintSDK.initialize(this, config)
```

### Configuration

#### `Config` class

SDK configuration options.

```kotlin
data class Config(
    val backendUrl: String = "http://localhost:3000",
    val enrollmentScans: Int = 5,
    val matchThreshold: Int = 60,
    val timeoutSeconds: Int = 30,
    val enableLogging: Boolean = false
)
```

**Properties:**
- `backendUrl`: Backend API URL for template storage
- `enrollmentScans`: Number of scans required for enrollment (default: 5)
- `matchThreshold`: Minimum score for successful match (0-100, default: 60)
- `timeoutSeconds`: Operation timeout in seconds (default: 30)
- `enableLogging`: Enable debug logging (default: false)

### Callbacks

#### `setCallback(callback: Callback)`

Set callback for operation results.

**Parameters:**
- `callback`: Callback interface implementation

**Example:**
```kotlin
sdk.setCallback(object : FingerprintSDK.Callback {
    override fun onEnrollSuccess(userId: Int) { }
    override fun onEnrollError(userId: Int, error: String) { }
    override fun onVerifySuccess(userId: Int, score: Int) { }
    override fun onVerifyError(userId: Int, error: String) { }
    override fun onIdentifySuccess(userId: Int, score: Int) { }
    override fun onIdentifyError(error: String) { }
})
```

#### `Callback` interface

```kotlin
interface Callback {
    fun onEnrollSuccess(userId: Int)
    fun onEnrollError(userId: Int, error: String)
    fun onVerifySuccess(userId: Int, score: Int)
    fun onVerifyError(userId: Int, error: String)
    fun onIdentifySuccess(userId: Int, score: Int)
    fun onIdentifyError(error: String)
}
```

### Operations

#### `enrollFingerprint(userId: Int)`

Enroll a fingerprint for a user.

**Parameters:**
- `userId`: User ID to enroll fingerprint for

**Returns:**
- `Unit` (results via callback)

**Callbacks:**
- `onEnrollSuccess(userId)` - Enrollment completed successfully
- `onEnrollError(userId, error)` - Enrollment failed

**Example:**
```kotlin
sdk.enrollFingerprint(userId = 123)
```

**Process:**
1. Requests USB permission if needed
2. Initializes USB device
3. Captures 5 fingerprint scans
4. Creates template from scans
5. Stores template in database
6. Calls success/error callback

#### `verifyFingerprint(userId: Int)`

Verify a fingerprint against stored template.

**Parameters:**
- `userId`: User ID to verify

**Returns:**
- `Unit` (results via callback)

**Callbacks:**
- `onVerifySuccess(userId, score)` - Verification successful (score 0-100)
- `onVerifyError(userId, error)` - Verification failed

**Example:**
```kotlin
sdk.verifyFingerprint(userId = 123)
```

**Process:**
1. Loads template from database for user
2. Captures fingerprint scan
3. Matches scan against template
4. Returns match result and score
5. Calls success/error callback

#### `identifyFingerprint()`

Identify user from fingerprint (1:N matching).

**Parameters:**
- None

**Returns:**
- `Unit` (results via callback)

**Callbacks:**
- `onIdentifySuccess(userId, score)` - User identified
- `onIdentifyError(error)` - Identification failed or no match

**Example:**
```kotlin
sdk.identifyFingerprint()
```

**Process:**
1. Loads all templates from database
2. Creates gallery from templates
3. Captures fingerprint scan
4. Matches scan against gallery
5. Returns matched user ID and score
6. Calls success/error callback

### Status & Control

#### `isReady(): Boolean`

Check if SDK is ready for operations.

**Returns:**
- `true` if device is initialized and ready
- `false` if device not ready or not connected

**Example:**
```kotlin
if (sdk.isReady()) {
    sdk.verifyFingerprint(userId)
} else {
    // Show message: Device not ready
}
```

#### `getStatus(): Status`

Get current SDK status.

**Returns:**
- `Status` enum value

**Status Values:**
- `Status.INITIALIZING` - SDK is initializing
- `Status.READY` - SDK is ready for operations
- `Status.ERROR` - SDK encountered an error
- `Status.DISCONNECTED` - USB device not connected

**Example:**
```kotlin
when (sdk.getStatus()) {
    FingerprintSDK.Status.READY -> { /* Ready */ }
    FingerprintSDK.Status.ERROR -> { /* Error */ }
    // ...
}
```

#### `cleanup()`

Clean up SDK resources.

**Should be called:**
- In `Activity.onDestroy()`
- When SDK is no longer needed

**Example:**
```kotlin
override fun onDestroy() {
    super.onDestroy()
    sdk.cleanup()
}
```

## Error Codes

Common error codes returned in callbacks:

| Code | Description |
|------|-------------|
| `USB_PERMISSION_DENIED` | USB permission not granted |
| `USB_DEVICE_NOT_FOUND` | USB device not connected |
| `DEVICE_INITIALIZATION_FAILED` | Failed to initialize device |
| `ENROLLMENT_FAILED` | Enrollment operation failed |
| `VERIFICATION_FAILED` | Verification operation failed |
| `IDENTIFICATION_FAILED` | Identification operation failed |
| `TEMPLATE_NOT_FOUND` | Template not found in database |
| `NETWORK_ERROR` | Network/API error |
| `TIMEOUT` | Operation timed out |
| `UNKNOWN_ERROR` | Unknown error occurred |

## Threading

All SDK operations are asynchronous and run on background threads. Callbacks are invoked on the main thread, so it's safe to update UI directly in callbacks.

## Lifecycle

SDK should be initialized in `onCreate()` and cleaned up in `onDestroy()`:

```kotlin
class MainActivity : AppCompatActivity() {
    private lateinit var sdk: FingerprintSDK
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        sdk = FingerprintSDK.initialize(this)
    }
    
    override fun onDestroy() {
        super.onDestroy()
        sdk.cleanup()
    }
}
```

## Best Practices

1. **Check readiness**: Always check `isReady()` before operations
2. **Handle errors**: Implement all callback methods
3. **Lifecycle**: Initialize in `onCreate()`, cleanup in `onDestroy()`
4. **Threading**: Don't block main thread, SDK handles threading
5. **Permissions**: SDK handles USB permissions automatically
6. **Error messages**: Show user-friendly error messages

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - Architecture overview
- [EXAMPLES.md](EXAMPLES.md) - Code examples
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Common issues


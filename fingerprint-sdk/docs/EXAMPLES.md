# Code Examples

Complete examples for using the Fingerprint SDK.

## Example 1: Basic Setup

```kotlin
import com.arkana.fingerprintsdk.FingerprintSDK

class MainActivity : AppCompatActivity() {
    private lateinit var sdk: FingerprintSDK
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Initialize SDK
        sdk = FingerprintSDK.initialize(this)
        
        // Set callback
        sdk.setCallback(object : FingerprintSDK.Callback {
            override fun onEnrollSuccess(userId: Int) {
                Toast.makeText(this@MainActivity, "Enrollment successful!", Toast.LENGTH_SHORT).show()
            }
            
            override fun onEnrollError(userId: Int, error: String) {
                Toast.makeText(this@MainActivity, "Enrollment failed: $error", Toast.LENGTH_LONG).show()
            }
            
            override fun onVerifySuccess(userId: Int, score: Int) {
                Toast.makeText(this@MainActivity, "Verified! Score: $score", Toast.LENGTH_SHORT).show()
            }
            
            override fun onVerifyError(userId: Int, error: String) {
                Toast.makeText(this@MainActivity, "Verification failed: $error", Toast.LENGTH_LONG).show()
            }
            
            override fun onIdentifySuccess(userId: Int, score: Int) {
                Toast.makeText(this@MainActivity, "User $userId identified! Score: $score", Toast.LENGTH_SHORT).show()
            }
            
            override fun onIdentifyError(error: String) {
                Toast.makeText(this@MainActivity, "Identification failed: $error", Toast.LENGTH_LONG).show()
            }
        })
    }
    
    override fun onDestroy() {
        super.onDestroy()
        sdk.cleanup()
    }
}
```

## Example 2: Enrollment with Progress

```kotlin
class EnrollmentActivity : AppCompatActivity() {
    private lateinit var sdk: FingerprintSDK
    private var currentScan = 0
    private val totalScans = 5
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        sdk = FingerprintSDK.initialize(this)
        sdk.setCallback(object : FingerprintSDK.Callback {
            override fun onEnrollSuccess(userId: Int) {
                runOnUiThread {
                    progressBar.progress = 100
                    statusText.text = "Enrollment complete!"
                }
            }
            
            override fun onEnrollError(userId: Int, error: String) {
                runOnUiThread {
                    statusText.text = "Error: $error"
                }
            }
            
            // ... other callbacks
        })
    }
    
    fun startEnrollment(userId: Int) {
        currentScan = 0
        progressBar.progress = 0
        statusText.text = "Place finger on reader (1/$totalScans)..."
        sdk.enrollFingerprint(userId)
    }
}
```

## Example 3: Verification with Retry

```kotlin
class VerificationActivity : AppCompatActivity() {
    private lateinit var sdk: FingerprintSDK
    private var retryCount = 0
    private val maxRetries = 3
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        sdk = FingerprintSDK.initialize(this)
        sdk.setCallback(object : FingerprintSDK.Callback {
            override fun onVerifySuccess(userId: Int, score: Int) {
                retryCount = 0
                showSuccess("Verified! Score: $score")
            }
            
            override fun onVerifyError(userId: Int, error: String) {
                if (retryCount < maxRetries) {
                    retryCount++
                    showMessage("Retry $retryCount/$maxRetries...")
                    Handler(Looper.getMainLooper()).postDelayed({
                        sdk.verifyFingerprint(userId)
                    }, 1000)
                } else {
                    showError("Verification failed after $maxRetries attempts")
                    retryCount = 0
                }
            }
            
            // ... other callbacks
        })
    }
    
    fun verifyUser(userId: Int) {
        retryCount = 0
        sdk.verifyFingerprint(userId)
    }
}
```

## Example 4: Identification with User List

```kotlin
class IdentificationActivity : AppCompatActivity() {
    private lateinit var sdk: FingerprintSDK
    private val userList = mutableListOf<User>()
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        sdk = FingerprintSDK.initialize(this)
        sdk.setCallback(object : FingerprintSDK.Callback {
            override fun onIdentifySuccess(userId: Int, score: Int) {
                val user = userList.find { it.id == userId }
                if (user != null) {
                    showUserInfo(user, score)
                } else {
                    showError("User not found")
                }
            }
            
            override fun onIdentifyError(error: String) {
                showError("Identification failed: $error")
            }
            
            // ... other callbacks
        })
        
        loadUserList()
    }
    
    fun identifyUser() {
        if (sdk.isReady()) {
            statusText.text = "Place finger on reader..."
            sdk.identifyFingerprint()
        } else {
            showError("Device not ready")
        }
    }
}
```

## Example 5: Custom Configuration

```kotlin
class CustomConfigActivity : AppCompatActivity() {
    private lateinit var sdk: FingerprintSDK
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Custom configuration
        val config = FingerprintSDK.Config(
            backendUrl = "https://api.production.com",
            enrollmentScans = 3, // Fewer scans for faster enrollment
            matchThreshold = 70, // Higher threshold for stricter matching
            timeoutSeconds = 60, // Longer timeout
            enableLogging = true // Enable debug logging
        )
        
        sdk = FingerprintSDK.initialize(this, config)
        // ... rest of setup
    }
}
```

## Example 6: Status Monitoring

```kotlin
class StatusMonitoringActivity : AppCompatActivity() {
    private lateinit var sdk: FingerprintSDK
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        sdk = FingerprintSDK.initialize(this)
        
        // Monitor status
        val handler = Handler(Looper.getMainLooper())
        val statusChecker = object : Runnable {
            override fun run() {
                updateStatus(sdk.getStatus())
                handler.postDelayed(this, 1000) // Check every second
            }
        }
        handler.post(statusChecker)
    }
    
    private fun updateStatus(status: FingerprintSDK.Status) {
        when (status) {
            FingerprintSDK.Status.INITIALIZING -> {
                statusText.text = "Initializing..."
                statusIndicator.setBackgroundColor(Color.YELLOW)
            }
            FingerprintSDK.Status.READY -> {
                statusText.text = "Ready"
                statusIndicator.setBackgroundColor(Color.GREEN)
            }
            FingerprintSDK.Status.ERROR -> {
                statusText.text = "Error"
                statusIndicator.setBackgroundColor(Color.RED)
            }
            FingerprintSDK.Status.DISCONNECTED -> {
                statusText.text = "Device disconnected"
                statusIndicator.setBackgroundColor(Color.GRAY)
            }
        }
    }
}
```

## Example 7: Error Handling

```kotlin
class ErrorHandlingActivity : AppCompatActivity() {
    private lateinit var sdk: FingerprintSDK
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        sdk = FingerprintSDK.initialize(this)
        sdk.setCallback(object : FingerprintSDK.Callback {
            override fun onEnrollError(userId: Int, error: String) {
                handleError("Enrollment", error)
            }
            
            override fun onVerifyError(userId: Int, error: String) {
                handleError("Verification", error)
            }
            
            override fun onIdentifyError(error: String) {
                handleError("Identification", error)
            }
            
            // ... success callbacks
        })
    }
    
    private fun handleError(operation: String, error: String) {
        when {
            error.contains("USB_PERMISSION_DENIED") -> {
                showDialog("Permission Required", "Please grant USB permission")
            }
            error.contains("USB_DEVICE_NOT_FOUND") -> {
                showDialog("Device Not Found", "Please connect the fingerprint reader")
            }
            error.contains("TIMEOUT") -> {
                showDialog("Timeout", "Operation timed out. Please try again.")
            }
            else -> {
                showDialog("Error", "$operation failed: $error")
            }
        }
    }
}
```

## Best Practices

1. **Always check readiness**: Use `isReady()` before operations
2. **Handle all callbacks**: Implement all callback methods
3. **Clean up resources**: Call `cleanup()` in `onDestroy()`
4. **Show user feedback**: Update UI in callbacks
5. **Handle errors gracefully**: Provide user-friendly error messages
6. **Monitor status**: Check status for better UX

## See Also

- [API_REFERENCE.md](API_REFERENCE.md) - Complete API documentation
- [ARCHITECTURE.md](ARCHITECTURE.md) - Architecture overview
- [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Common issues


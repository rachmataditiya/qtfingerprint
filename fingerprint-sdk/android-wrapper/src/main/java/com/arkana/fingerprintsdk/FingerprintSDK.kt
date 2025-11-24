package com.arkana.fingerprintsdk

import android.content.Context
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.util.Log
import java.util.concurrent.Executors

/**
 * Main SDK class for fingerprint operations.
 * 
 * Provides a clean, simple API for fingerprint enrollment, verification, and identification.
 * 
 * @example
 * ```kotlin
 * val sdk = FingerprintSDK.initialize(this)
 * sdk.setCallback(object : FingerprintSDK.Callback {
 *     override fun onEnrollSuccess(userId: Int) {
 *         // Handle success
 *     }
 *     // ... other callbacks
 * })
 * sdk.enrollFingerprint(userId = 123)
 * ```
 */
class FingerprintSDK private constructor(
    private val context: Context,
    private val config: Config
) {
    companion object {
        private const val TAG = "FingerprintSDK"
        
        @Volatile
        private var instance: FingerprintSDK? = null
        
        /**
         * Initialize SDK with default configuration.
         * 
         * @param context Android Context
         * @return FingerprintSDK instance
         * @throws SDKException if initialization fails
         */
        @JvmStatic
        fun initialize(context: Context): FingerprintSDK {
            return initialize(context, Config())
        }
        
        /**
         * Initialize SDK with custom configuration.
         * 
         * @param context Android Context
         * @param config SDK configuration
         * @return FingerprintSDK instance
         * @throws SDKException if initialization fails
         */
        @JvmStatic
        fun initialize(context: Context, config: Config): FingerprintSDK {
            return instance ?: synchronized(this) {
                instance ?: FingerprintSDK(context, config).also { instance = it }
            }
        }
    }
    
    private val executor = Executors.newSingleThreadExecutor()
    private var callback: Callback? = null
    private var status: Status = Status.INITIALIZING
    private val usbManager: UsbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
    
    init {
        // Load native library
        System.loadLibrary("fingerprint-sdk-native")
        
        // Initialize native service
        initializeNative()
    }
    
    /**
     * Set callback for operation results.
     * 
     * @param callback Callback interface implementation
     */
    fun setCallback(callback: Callback) {
        this.callback = callback
    }
    
    /**
     * Enroll fingerprint for a user.
     * 
     * This will:
     * 1. Request USB permission if needed
     * 2. Initialize USB device
     * 3. Capture 5 fingerprint scans
     * 4. Create template from scans
     * 5. Store template in database
     * 
     * Results are delivered via [Callback.onEnrollSuccess] or [Callback.onEnrollError].
     * 
     * @param userId User ID to enroll fingerprint for
     */
    fun enrollFingerprint(userId: Int) {
        if (!isReady()) {
            callback?.onEnrollError(userId, "SDK not ready. Device may not be connected.")
            return
        }
        
        executor.execute {
            try {
                val success = nativeEnroll(userId)
                if (success) {
                    callback?.onEnrollSuccess(userId)
                } else {
                    callback?.onEnrollError(userId, "Enrollment failed")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Enrollment error", e)
                callback?.onEnrollError(userId, e.message ?: "Unknown error")
            }
        }
    }
    
    /**
     * Verify fingerprint for a user.
     * 
     * This will:
     * 1. Load stored template from database
     * 2. Capture fingerprint scan
     * 3. Match scan against template
     * 4. Return match result and score
     * 
     * Results are delivered via [Callback.onVerifySuccess] or [Callback.onVerifyError].
     * 
     * @param userId User ID to verify
     */
    fun verifyFingerprint(userId: Int) {
        if (!isReady()) {
            callback?.onVerifyError(userId, "SDK not ready. Device may not be connected.")
            return
        }
        
        executor.execute {
            try {
                val score = nativeVerify(userId)
                if (score >= 0 && score >= config.matchThreshold) {
                    callback?.onVerifySuccess(userId, score)
                } else {
                    callback?.onVerifyError(userId, "Verification failed. Score: $score")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Verification error", e)
                callback?.onVerifyError(userId, e.message ?: "Unknown error")
            }
        }
    }
    
    /**
     * Identify user from fingerprint (1:N matching).
     * 
     * This will:
     * 1. Load all templates from database
     * 2. Create gallery from templates
     * 3. Capture fingerprint scan
     * 4. Match scan against gallery
     * 5. Return matched user ID and score
     * 
     * Results are delivered via [Callback.onIdentifySuccess] or [Callback.onIdentifyError].
     */
    fun identifyFingerprint() {
        if (!isReady()) {
            callback?.onIdentifyError("SDK not ready. Device may not be connected.")
            return
        }
        
        executor.execute {
            try {
                val userId = nativeIdentify()
                if (userId >= 0) {
                    // Get score (would need to modify native method to return both)
                    callback?.onIdentifySuccess(userId, 0) // TODO: Get actual score
                } else {
                    callback?.onIdentifyError("No matching user found")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Identification error", e)
                callback?.onIdentifyError(e.message ?: "Unknown error")
            }
        }
    }
    
    /**
     * Check if SDK is ready for operations.
     * 
     * @return true if device is initialized and ready, false otherwise
     */
    fun isReady(): Boolean {
        return status == Status.READY
    }
    
    /**
     * Get current SDK status.
     * 
     * @return Current status
     */
    fun getStatus(): Status {
        return status
    }
    
    /**
     * Clean up SDK resources.
     * 
     * Should be called in Activity.onDestroy() or when SDK is no longer needed.
     */
    fun cleanup() {
        executor.execute {
            nativeCleanup()
        }
        executor.shutdown()
        instance = null
    }
    
    // Native methods
    private external fun initializeNative(): Boolean
    private external fun nativeEnroll(userId: Int): Boolean
    private external fun nativeVerify(userId: Int): Int
    private external fun nativeIdentify(): Int
    private external fun nativeCleanup()
    
    /**
     * Callback interface for operation results.
     */
    interface Callback {
        /**
         * Called when enrollment succeeds.
         * 
         * @param userId User ID that was enrolled
         */
        fun onEnrollSuccess(userId: Int)
        
        /**
         * Called when enrollment fails.
         * 
         * @param userId User ID that failed to enroll
         * @param error Error message
         */
        fun onEnrollError(userId: Int, error: String)
        
        /**
         * Called when verification succeeds.
         * 
         * @param userId User ID that was verified
         * @param score Match score (0-100)
         */
        fun onVerifySuccess(userId: Int, score: Int)
        
        /**
         * Called when verification fails.
         * 
         * @param userId User ID that failed to verify
         * @param error Error message
         */
        fun onVerifyError(userId: Int, error: String)
        
        /**
         * Called when identification succeeds.
         * 
         * @param userId Identified user ID
         * @param score Match score (0-100)
         */
        fun onIdentifySuccess(userId: Int, score: Int)
        
        /**
         * Called when identification fails.
         * 
         * @param error Error message
         */
        fun onIdentifyError(error: String)
    }
    
    /**
     * SDK status.
     */
    enum class Status {
        INITIALIZING,
        READY,
        ERROR,
        DISCONNECTED
    }
    
    /**
     * SDK configuration.
     */
    data class Config(
        val backendUrl: String = "http://localhost:3000",
        val enrollmentScans: Int = 5,
        val matchThreshold: Int = 60,
        val timeoutSeconds: Int = 30,
        val enableLogging: Boolean = false
    )
}


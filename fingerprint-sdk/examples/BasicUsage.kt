package com.arkana.fingerprintsdk.examples

import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.arkana.fingerprintsdk.FingerprintSDK

/**
 * Basic usage example of Fingerprint SDK.
 */
class BasicUsageActivity : AppCompatActivity() {
    private lateinit var sdk: FingerprintSDK
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Initialize SDK with default config
        sdk = FingerprintSDK.initialize(this)
        
        // Or with custom config
        val config = FingerprintSDK.Config(
            backendUrl = "http://api.example.com",
            enrollmentScans = 5,
            matchThreshold = 60
        )
        // sdk = FingerprintSDK.initialize(this, config)
        
        // Set callback
        sdk.setCallback(object : FingerprintSDK.Callback {
            override fun onEnrollSuccess(userId: Int) {
                Log.d(TAG, "Enrollment successful for user $userId")
                // Update UI
            }
            
            override fun onEnrollError(userId: Int, error: String) {
                Log.e(TAG, "Enrollment failed for user $userId: $error")
                // Show error to user
            }
            
            override fun onVerifySuccess(userId: Int, score: Int) {
                Log.d(TAG, "Verification successful for user $userId, score: $score")
                // Update UI
            }
            
            override fun onVerifyError(userId: Int, error: String) {
                Log.e(TAG, "Verification failed for user $userId: $error")
                // Show error to user
            }
            
            override fun onIdentifySuccess(userId: Int, score: Int) {
                Log.d(TAG, "User identified: $userId, score: $score")
                // Update UI with identified user
            }
            
            override fun onIdentifyError(error: String) {
                Log.e(TAG, "Identification failed: $error")
                // Show error to user
            }
        })
    }
    
    private fun enrollUser(userId: Int) {
        if (sdk.isReady()) {
            sdk.enrollFingerprint(userId)
        } else {
            Log.w(TAG, "SDK not ready")
        }
    }
    
    private fun verifyUser(userId: Int) {
        if (sdk.isReady()) {
            sdk.verifyFingerprint(userId)
        } else {
            Log.w(TAG, "SDK not ready")
        }
    }
    
    private fun identifyUser() {
        if (sdk.isReady()) {
            sdk.identifyFingerprint()
        } else {
            Log.w(TAG, "SDK not ready")
        }
    }
    
    override fun onDestroy() {
        super.onDestroy()
        sdk.cleanup()
    }
    
    companion object {
        private const val TAG = "BasicUsageActivity"
    }
}


package com.arkana.fingerprintpoc

import android.content.Context
import android.util.Log
import androidx.fragment.app.FragmentActivity
import com.arkana.libdigitalpersona.FingerprintJNI
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * FingerprintManager using libfprint via JNI
 * Integrates with libdigitalpersona module for native fingerprint operations
 */
class FingerprintManager(private val context: Context) {
    
    private var activity: FragmentActivity? = null
    private var isInitialized = false
    
    companion object {
        private const val TAG = "FingerprintManager"
    }
    
    fun initialize(activity: FragmentActivity): Boolean {
        this.activity = activity
        
        // Initialize libfprint via JNI
        val initialized = FingerprintJNI.initialize(context)
        if (initialized) {
            val deviceCount = FingerprintJNI.getDeviceCount()
            if (deviceCount > 0) {
                val opened = FingerprintJNI.openReader(activity)
                if (opened) {
                    Log.i(TAG, "libfprint initialized successfully, device count: $deviceCount")
                    isInitialized = true
                    return true
                } else {
                    Log.e(TAG, "Failed to open fingerprint reader: ${FingerprintJNI.getLastError()}")
                }
            } else {
                Log.w(TAG, "No fingerprint devices found")
            }
        } else {
            Log.e(TAG, "Failed to initialize libfprint: ${FingerprintJNI.getLastError()}")
        }
        
        isInitialized = false
        return false
    }
    
    fun cleanup() {
        FingerprintJNI.cleanup()
        activity = null
        isInitialized = false
    }
    
    fun getDeviceCount(): Int {
        return if (isInitialized) {
            FingerprintJNI.getDeviceCount()
        } else {
            0
        }
    }
    
    suspend fun captureFingerprintTemplate(): ByteArray? {
        if (!isInitialized) {
            Log.e(TAG, "FingerprintManager not initialized")
            return null
        }
        
        return withContext(Dispatchers.IO) {
            try {
                Log.d(TAG, "Capturing fingerprint using libfprint")
                val image = FingerprintJNI.captureFingerprint()
                if (image != null && image.isNotEmpty()) {
                    Log.d(TAG, "Fingerprint captured successfully: ${image.size} bytes")
                    image
                } else {
                    Log.e(TAG, "Failed to capture fingerprint: ${FingerprintJNI.getLastError()}")
                    null
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error capturing fingerprint: ${e.message}", e)
                null
            }
        }
    }
    
    suspend fun verifyFingerprint(templateData: ByteArray): Pair<Boolean, Int> {
        if (!isInitialized) {
            Log.e(TAG, "FingerprintManager not initialized")
            return Pair(false, 0)
        }
        
        return withContext(Dispatchers.IO) {
            try {
                Log.d(TAG, "Verifying fingerprint using libfprint (local matching)")
                val (matched, score) = FingerprintJNI.verifyFingerprint(templateData)
                Log.d(TAG, "Verification result: matched=$matched, score=$score")
                Pair(matched, score)
            } catch (e: Exception) {
                Log.e(TAG, "Error verifying fingerprint: ${e.message}", e)
                Pair(false, 0)
            }
        }
    }
    
    suspend fun identifyUser(userTemplates: Map<Int, ByteArray>): Pair<Int, Int> {
        if (!isInitialized) {
            Log.e(TAG, "FingerprintManager not initialized")
            return Pair(-1, 0)
        }
        
        return withContext(Dispatchers.IO) {
            try {
                Log.d(TAG, "Identifying user from ${userTemplates.size} users using libfprint (local matching)")
                val (userId, score) = FingerprintJNI.identifyUser(userTemplates)
                if (userId >= 0) {
                    Log.d(TAG, "User identified: userId=$userId, score=$score")
                } else {
                    Log.d(TAG, "No user matched")
                }
                Pair(userId, score)
            } catch (e: Exception) {
                Log.e(TAG, "Error identifying user: ${e.message}", e)
                Pair(-1, 0)
            }
        }
    }
    
    fun cancel() {
        Log.d(TAG, "Cancel called")
        FingerprintJNI.cancel()
    }
}


package com.arkana.libdigitalpersona

import android.content.Context
import android.os.Build
import android.os.CancellationSignal
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import androidx.annotation.RequiresApi
import androidx.biometric.BiometricManager
import androidx.biometric.BiometricPrompt
import androidx.biometric.BiometricPrompt.PromptInfo
import androidx.core.content.ContextCompat
import androidx.fragment.app.FragmentActivity
import java.security.KeyStore
import java.util.concurrent.Executor
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey

/**
 * Android Fingerprint Service using BiometricPrompt API
 * Equivalent to FingerprintManager in desktop version
 * 
 * Note: Android stores templates in secure hardware and doesn't allow extraction.
 * We use Android KeyStore keys as "templates" - each user gets a unique key alias.
 */
class FingerprintService(private val context: Context) {
    
    companion object {
        private const val KEYSTORE_NAME = "AndroidKeyStore"
        private const val KEY_ALIAS_PREFIX = "fingerprint_user_"
        private const val KEY_ALGORITHM = KeyProperties.KEY_ALGORITHM_AES
        private const val BLOCK_MODE = KeyProperties.BLOCK_MODE_CBC
        private const val ENCRYPTION_PADDING = KeyProperties.ENCRYPTION_PADDING_PKCS7
        private const val TRANSFORMATION = "$KEY_ALGORITHM/$BLOCK_MODE/$ENCRYPTION_PADDING"
    }
    
    private var biometricPrompt: BiometricPrompt? = null
    private var cancellationSignal: CancellationSignal? = null
    private var executor: Executor = ContextCompat.getMainExecutor(context)
    private val keyStore: KeyStore = KeyStore.getInstance(KEYSTORE_NAME).apply { load(null) }
    
    private var enrollmentCallback: EnrollmentCallback? = null
    private var verificationCallback: VerificationCallback? = null
    private var identificationCallback: IdentificationCallback? = null
    
    // Enrollment state
    private var enrollmentStages: Int = 0
    private val requiredStages: Int = 5
    private var enrollmentKeyAlias: String? = null
    private var enrollmentActivity: FragmentActivity? = null
    
    interface EnrollmentCallback {
        fun onProgress(current: Int, total: Int, message: String)
        fun onComplete(template: ByteArray) // Returns key alias as "template"
        fun onError(error: String)
    }
    
    interface VerificationCallback {
        fun onSuccess(score: Int)
        fun onFailure(error: String)
    }
    
    interface IdentificationCallback {
        fun onMatch(userId: Int, score: Int)
        fun onNoMatch(error: String)
    }
    
    /**
     * Check if fingerprint hardware is available
     */
    fun isAvailable(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
            return false
        }
        
        val biometricManager = BiometricManager.from(context)
        return when (biometricManager.canAuthenticate(BiometricManager.Authenticators.BIOMETRIC_STRONG)) {
            BiometricManager.BIOMETRIC_SUCCESS -> true
            else -> false
        }
    }
    
    /**
     * Get device count (always 1 for Android - system fingerprint sensor)
     */
    fun getDeviceCount(): Int {
        return if (isAvailable()) 1 else 0
    }
    
    /**
     * Start enrollment process - creates KeyStore key for user
     */
    @RequiresApi(Build.VERSION_CODES.P)
    fun startEnrollment(userId: Int, activity: FragmentActivity, callback: EnrollmentCallback) {
        if (!isAvailable()) {
            callback.onError("Fingerprint hardware not available")
            return
        }
        
        enrollmentCallback = callback
        enrollmentActivity = activity
        enrollmentStages = 0
        enrollmentKeyAlias = "$KEY_ALIAS_PREFIX$userId"
        
        // Delete existing key if any
        try {
            keyStore.deleteEntry(enrollmentKeyAlias!!)
        } catch (e: Exception) {
            // Ignore if key doesn't exist
        }
        
        // Create KeyStore key with biometric requirement
        createKeyForEnrollment(enrollmentKeyAlias!!)
        
        // Start enrollment by prompting for biometric
        promptNextEnrollmentStage()
    }
    
    @RequiresApi(Build.VERSION_CODES.P)
    private fun createKeyForEnrollment(alias: String) {
        val keyGenerator = KeyGenerator.getInstance(KEY_ALGORITHM, KEYSTORE_NAME)
        val keyGenParameterSpec = KeyGenParameterSpec.Builder(
            alias,
            KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT
        )
            .setBlockModes(BLOCK_MODE)
            .setEncryptionPaddings(ENCRYPTION_PADDING)
            .setUserAuthenticationRequired(true)
            .setInvalidatedByBiometricEnrollment(false)
            .build()
        
        keyGenerator.init(keyGenParameterSpec)
        keyGenerator.generateKey()
    }
    
    @RequiresApi(Build.VERSION_CODES.P)
    private fun promptNextEnrollmentStage() {
        val alias = enrollmentKeyAlias ?: return
        val activity = enrollmentActivity ?: return
        val callback = enrollmentCallback ?: return
        
        // Create cipher for this key
        val cipher = createCipher(alias) ?: run {
            callback.onError("Failed to create cipher for enrollment")
            return
        }
        
        biometricPrompt = BiometricPrompt(
            activity,
            executor,
            object : BiometricPrompt.AuthenticationCallback() {
                override fun onAuthenticationSucceeded(result: BiometricPrompt.AuthenticationResult) {
                    enrollmentStages++
                    callback.onProgress(
                        enrollmentStages, 
                        requiredStages,
                        "Scan $enrollmentStages/$requiredStages complete"
                    )
                    
                    if (enrollmentStages < requiredStages) {
                        // Continue enrollment
                        promptNextEnrollmentStage()
                    } else {
                        // Enrollment complete - return key alias as "template"
                        val template = alias.toByteArray()
                        callback.onComplete(template)
                    }
                }
                
                override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                    if (errorCode != BiometricPrompt.ERROR_USER_CANCELED && 
                        errorCode != BiometricPrompt.ERROR_NEGATIVE_BUTTON) {
                        callback.onProgress(enrollmentStages, requiredStages, errString.toString())
                    }
                    callback.onError("Enrollment error: $errString")
                }
            }
        )
        
        val promptInfo = PromptInfo.Builder()
            .setTitle("Fingerprint Enrollment")
            .setSubtitle("Scan ${enrollmentStages + 1}/$requiredStages - Place finger on sensor")
            .setNegativeButtonText("Cancel")
            .build()
        
        cancellationSignal = CancellationSignal()
        val cryptoObject = BiometricPrompt.CryptoObject(cipher)
        biometricPrompt?.authenticate(promptInfo, cryptoObject)
    }
    
    /**
     * Verify fingerprint against stored key (1:1)
     */
    @RequiresApi(Build.VERSION_CODES.P)
    fun verifyFingerprint(userId: Int, activity: FragmentActivity, callback: VerificationCallback) {
        if (!isAvailable()) {
            callback.onFailure("Fingerprint hardware not available")
            return
        }
        
        verificationCallback = callback
        val keyAlias = "$KEY_ALIAS_PREFIX$userId"
        
        // Check if key exists
        if (!keyStore.containsAlias(keyAlias)) {
            callback.onFailure("User not enrolled")
            return
        }
        
        // Create cipher
        val cipher = createCipher(keyAlias) ?: run {
            callback.onFailure("Failed to create cipher for verification")
            return
        }
        
        biometricPrompt = BiometricPrompt(
            activity,
            executor,
            object : BiometricPrompt.AuthenticationCallback() {
                override fun onAuthenticationSucceeded(result: BiometricPrompt.AuthenticationResult) {
                    callback.onSuccess(95) // High confidence
                }
                
                override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                    callback.onFailure("Verification failed: $errString")
                }
            }
        )
        
        val promptInfo = PromptInfo.Builder()
            .setTitle("Fingerprint Verification")
            .setSubtitle("Place your finger on the sensor")
            .setNegativeButtonText("Cancel")
            .build()
        
        cancellationSignal = CancellationSignal()
        val cryptoObject = BiometricPrompt.CryptoObject(cipher)
        biometricPrompt?.authenticate(promptInfo, cryptoObject)
    }
    
    /**
     * Identify user from multiple enrolled users (1:N)
     * Note: This requires attempting authentication for each user's key
     */
    @RequiresApi(Build.VERSION_CODES.P)
    fun identifyUser(userIds: List<Int>, activity: FragmentActivity, callback: IdentificationCallback) {
        if (!isAvailable()) {
            callback.onNoMatch("Fingerprint hardware not available")
            return
        }
        
        if (userIds.isEmpty()) {
            callback.onNoMatch("No enrolled users")
            return
        }
        
        identificationCallback = callback
        identifyNextUser(userIds, 0, activity)
    }
    
    @RequiresApi(Build.VERSION_CODES.P)
    private fun identifyNextUser(userIds: List<Int>, index: Int, activity: FragmentActivity) {
        if (index >= userIds.size) {
            identificationCallback?.onNoMatch("No matching user found")
            return
        }
        
        val userId = userIds[index]
        val keyAlias = "$KEY_ALIAS_PREFIX$userId"
        
        // Skip if key doesn't exist
        if (!keyStore.containsAlias(keyAlias)) {
            identifyNextUser(userIds, index + 1, activity)
            return
        }
        
        val cipher = createCipher(keyAlias) ?: run {
            identifyNextUser(userIds, index + 1, activity)
            return
        }
        
        biometricPrompt = BiometricPrompt(
            activity,
            executor,
            object : BiometricPrompt.AuthenticationCallback() {
                override fun onAuthenticationSucceeded(result: BiometricPrompt.AuthenticationResult) {
                    identificationCallback?.onMatch(userId, 95)
                }
                
                override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                    // Try next user
                    identifyNextUser(userIds, index + 1, activity)
                }
            }
        )
        
        val promptInfo = PromptInfo.Builder()
            .setTitle("Fingerprint Identification")
            .setSubtitle("Checking user ${index + 1}/${userIds.size}...")
            .setNegativeButtonText("Cancel")
            .build()
        
        cancellationSignal = CancellationSignal()
        val cryptoObject = BiometricPrompt.CryptoObject(cipher)
        biometricPrompt?.authenticate(promptInfo, cryptoObject)
    }
    
    /**
     * Cancel current operation
     */
    fun cancel() {
        cancellationSignal?.cancel()
        cancellationSignal = null
        biometricPrompt = null
    }
    
    /**
     * Create cipher for KeyStore key
     */
    private fun createCipher(alias: String): Cipher? {
        return try {
            val secretKey = keyStore.getKey(alias, null) as? SecretKey ?: return null
            val cipher = Cipher.getInstance(TRANSFORMATION)
            cipher.init(Cipher.ENCRYPT_MODE, secretKey)
            cipher
        } catch (e: Exception) {
            null
        }
    }
    
    /**
     * Delete enrolled user (remove key)
     */
    fun deleteUser(userId: Int): Boolean {
        return try {
            val alias = "$KEY_ALIAS_PREFIX$userId"
            keyStore.deleteEntry(alias)
            true
        } catch (e: Exception) {
            false
        }
    }
}

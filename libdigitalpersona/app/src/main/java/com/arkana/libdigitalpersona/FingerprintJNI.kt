package com.arkana.libdigitalpersona

import android.content.Context

/**
 * JNI Bridge class for C++ to Java communication
 * Exposes FingerprintService to native C++ code
 */
object FingerprintJNI {
    
    private var fingerprintService: FingerprintService? = null
    private var currentContext: Context? = null
    @JvmStatic
    var nativeInstance: Long = 0 // Made public for access from FingerprintManager
    
    // Native methods for C++ instance management
    @JvmStatic
    private external fun createNativeInstance(context: Any): Long
    
    @JvmStatic
    private external fun destroyNativeInstance(nativePtr: Long)
    
    
    // Native methods for fingerprint operations
    @JvmStatic
    private external fun nativeIsAvailable(nativePtr: Long): Boolean
    
    @JvmStatic
    private external fun nativeGetDeviceCount(nativePtr: Long): Int
    
    @JvmStatic
    private external fun nativeSetUsbFileDescriptor(nativePtr: Long, fd: Int): Boolean
    
    @JvmStatic
    private external fun nativeOpenReader(nativePtr: Long, activity: Any): Boolean
    
    @JvmStatic
    private external fun nativeStartEnrollment(nativePtr: Long, userId: Int): Boolean
    
    @JvmStatic
    private external fun nativeVerifyFingerprint(nativePtr: Long, userId: Int, scoreOut: IntArray): Boolean
    
    @JvmStatic
    private external fun nativeIdentifyUser(nativePtr: Long, userIds: IntArray, scoreOut: IntArray): Int
    
    @JvmStatic
    private external fun nativeCancel(nativePtr: Long)
    
    @JvmStatic
    private external fun nativeGetLastError(nativePtr: Long): String
    
    @JvmStatic
    private external fun nativeCaptureFingerprint(nativePtr: Long): ByteArray?
    
    @JvmStatic
    private external fun nativeMatchWithTemplate(nativePtr: Long, templateData: ByteArray, resultOut: IntArray): Boolean
    
    @JvmStatic
    private external fun nativeIdentifyWithTemplates(nativePtr: Long, userIds: Array<IntArray>, templates: Array<ByteArray>, scoreOut: IntArray): Int
    
    // Callbacks from C++ to Java/Kotlin
    @JvmStatic
    private external fun onEnrollmentProgress(current: Int, total: Int, message: String)
    
    @JvmStatic
    private external fun onEnrollmentComplete(template: ByteArray)
    
    @JvmStatic
    private external fun onEnrollmentError(error: String)
    
    @JvmStatic
    private external fun onVerificationSuccess(score: Int)
    
    @JvmStatic
    private external fun onVerificationFailure(error: String)
    
    @JvmStatic
    private external fun onIdentificationMatch(userId: Int, score: Int)
    
    @JvmStatic
    private external fun onIdentificationNoMatch(error: String)
    
    @JvmStatic
    fun initialize(context: Context): Boolean {
        currentContext = context.applicationContext
        fingerprintService = FingerprintService(context.applicationContext)
        
        // Create native instance
        nativeInstance = createNativeInstance(context)
        return nativeInstance != 0L
    }
    
    @JvmStatic
    fun cleanup() {
        if (nativeInstance != 0L) {
            destroyNativeInstance(nativeInstance)
            nativeInstance = 0
        }
        fingerprintService?.cancel()
        fingerprintService = null
        currentContext = null
    }
    
    @JvmStatic
    fun isAvailable(): Boolean {
        return if (nativeInstance != 0L) {
            nativeIsAvailable(nativeInstance)
        } else {
            fingerprintService?.isAvailable() ?: false
        }
    }
    
    @JvmStatic
    fun getDeviceCount(): Int {
        return if (nativeInstance != 0L) {
            nativeGetDeviceCount(nativeInstance)
        } else {
            fingerprintService?.getDeviceCount() ?: 0
        }
    }
    
    @JvmStatic
    fun setUsbFileDescriptor(fd: Int): Boolean {
        if (nativeInstance == 0L) return false
        return nativeSetUsbFileDescriptor(nativeInstance, fd)
    }
    
    @JvmStatic
    fun openReader(activity: Any): Boolean {
        if (nativeInstance == 0L) return false
        return nativeOpenReader(nativeInstance, activity)
    }
    
    @JvmStatic
    fun startEnrollment(userId: Int): Boolean {
        if (nativeInstance == 0L) return false
        return nativeStartEnrollment(nativeInstance, userId)
    }
    
    @JvmStatic
    fun verifyFingerprint(templateData: ByteArray): Pair<Boolean, Int> {
        if (nativeInstance == 0L) return Pair(false, 0)
        
        val resultOut = IntArray(2) // [matched, score]
        val success = nativeMatchWithTemplate(nativeInstance, templateData, resultOut)
        
        if (!success) {
            return Pair(false, 0)
        }
        
        val matched = resultOut[0] != 0
        val score = resultOut[1]
        return Pair(matched, score)
    }
    
    @JvmStatic
    fun identifyUser(userTemplates: Map<Int, ByteArray>): Pair<Int, Int> {
        if (nativeInstance == 0L) return Pair(-1, 0)
        
        if (userTemplates.isEmpty()) {
            return Pair(-1, 0)
        }
        
        // Convert map to arrays
        val userIds = userTemplates.keys.map { intArrayOf(it) }.toTypedArray()
        val templates = userTemplates.values.toTypedArray()
        
        val scoreOut = IntArray(1)
        val matchedUserId = nativeIdentifyWithTemplates(nativeInstance, userIds, templates, scoreOut)
        
        return Pair(matchedUserId, scoreOut[0])
    }
    
    @JvmStatic
    fun cancel() {
        if (nativeInstance != 0L) {
            nativeCancel(nativeInstance)
        }
        fingerprintService?.cancel()
    }
    
    @JvmStatic
    fun deleteUser(userId: Int): Boolean {
        return fingerprintService?.deleteUser(userId) ?: false
    }
    
    @JvmStatic
    fun getLastError(): String {
        return if (nativeInstance != 0L) {
            nativeGetLastError(nativeInstance)
        } else {
            "Not initialized"
        }
    }
    
    @JvmStatic
    fun captureFingerprint(): ByteArray? {
        return if (nativeInstance != 0L) {
            nativeCaptureFingerprint(nativeInstance)
        } else {
            null
        }
    }
    
    init {
        System.loadLibrary("libdigitalpersona")
    }
}


package com.arkana.fingerprint.sdk.capture.native

import android.util.Log
import com.arkana.fingerprint.sdk.model.CaptureResult
import com.arkana.fingerprint.sdk.util.FingerError

/**
 * JNI bridge to native libfprint operations.
 */
object LibfprintNative {
    private const val TAG = "LibfprintNative"
    
    init {
        System.loadLibrary("arkana_fprint")
        Log.d(TAG, "Native library loaded")
    }

    /**
     * Open USB device.
     * 
     * @param fileDescriptor USB file descriptor from UsbDeviceConnection
     * @return true if successful
     */
    external fun openDevice(fileDescriptor: Int): Boolean

    /**
     * Close USB device.
     */
    external fun closeDevice()

    /**
     * Capture fingerprint.
     * 
     * @param timeoutMs Timeout in milliseconds
     * @return CaptureResult
     */
    fun capture(timeoutMs: Long): CaptureResult {
        Log.d(TAG, "capture: timeoutMs=$timeoutMs")
        val result = nativeCapture(timeoutMs)
        return if (result != null) {
            Log.d(TAG, "capture: Success, template size=${result.size}")
            CaptureResult.Success(result)
        } else {
            Log.e(TAG, "capture: Failed")
            CaptureResult.Error(FingerError.CAPTURE_FAILED)
        }
    }

    private external fun nativeCapture(timeoutMs: Long): ByteArray?
    
    /**
     * Enroll fingerprint (full enrollment with multiple scans).
     * 
     * @return CaptureResult with FP3 template
     */
    fun enroll(): CaptureResult {
        Log.d(TAG, "enroll: Starting enrollment...")
        val result = nativeEnroll()
        return if (result != null) {
            Log.d(TAG, "enroll: Success, template size=${result.size}")
            CaptureResult.Success(result)
        } else {
            Log.e(TAG, "enroll: Failed")
            CaptureResult.Error(FingerError.CAPTURE_FAILED)
        }
    }

    private external fun nativeEnroll(): ByteArray?

    /**
     * Match two templates.
     * matchWithTemplate will capture current fingerprint and match with stored template in one step.
     * 
     * @param template1 First template (ignored, native captures live)
     * @param template2 Second template (stored template from database)
     * @return Match score (0.0-1.0)
     */
    external fun match(template1: ByteArray, template2: ByteArray): Float
    
    /**
     * Identify user from multiple templates (1:N matching).
     * identifyUser will capture current fingerprint and match against all templates in one step.
     * 
     * @param userIds Array of user IDs (one per template)
     * @param fingers Array of finger names (one per template)
     * @param templates Array of templates (FP3 format)
     * @param scoreOut Output array for score [0]
     * @param matchedIndexOut Output array for matched template index [0]
     * @return Matched template index, or -1 if no match
     */
    fun identify(
        userIds: IntArray,
        fingers: Array<String>,
        templates: Array<ByteArray>,
        scoreOut: IntArray,
        matchedIndexOut: IntArray
    ): Int {
        return nativeIdentify(userIds, fingers, templates, scoreOut, matchedIndexOut)
    }
    
    private external fun nativeIdentify(
        userIds: IntArray,
        fingers: Array<String>,
        templates: Array<ByteArray>,
        scoreOut: IntArray,
        matchedIndexOut: IntArray
    ): Int
}


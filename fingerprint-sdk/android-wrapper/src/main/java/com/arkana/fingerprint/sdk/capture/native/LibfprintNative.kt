package com.arkana.fingerprint.sdk.capture.native

import android.hardware.usb.UsbDevice
import com.arkana.fingerprint.sdk.model.CaptureResult
import com.arkana.fingerprint.sdk.util.FingerError

/**
 * JNI bridge to native libfprint operations.
 */
object LibfprintNative {
    init {
        System.loadLibrary("arkana_fprint")
    }

    /**
     * Open USB device.
     * 
     * @param device USB device
     * @return true if successful
     */
    external fun openDevice(device: UsbDevice): Boolean

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
        val result = nativeCapture(timeoutMs)
        return if (result != null) {
            CaptureResult.Success(result)
        } else {
            CaptureResult.Error(FingerError.CAPTURE_FAILED)
        }
    }

    private external fun nativeCapture(timeoutMs: Long): ByteArray?

    /**
     * Match two templates.
     * 
     * @param template1 First template
     * @param template2 Second template
     * @return Match score (0.0-1.0)
     */
    external fun match(template1: ByteArray, template2: ByteArray): Float
}


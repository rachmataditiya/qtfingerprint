package com.arkana.fingerprint.sdk.capture

import android.content.Context
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import com.arkana.fingerprint.sdk.model.CaptureResult
import com.arkana.fingerprint.sdk.util.FingerError
import com.arkana.fingerprint.sdk.capture.native.LibfprintNative

/**
 * Manages fingerprint capture operations.
 * 
 * Handles:
 * - USB device initialization
 * - Fingerprint capture
 * - Device lifecycle
 */
class CaptureManager(private val context: Context) {
    private val usbManager: UsbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
    private var isDeviceOpen = false

    /**
     * Capture fingerprint once.
     * 
     * @param timeoutMs Timeout in milliseconds
     * @return CaptureResult
     */
    suspend fun captureOnce(timeoutMs: Long): CaptureResult {
        // Ensure device is open
        if (!isDeviceOpen) {
            val openResult = openDevice()
            if (openResult is CaptureResult.Error) {
                return openResult
            }
        }

        // Capture via native
        return LibfprintNative.capture(timeoutMs)
    }

    /**
     * Open USB device.
     */
    private suspend fun openDevice(): CaptureResult {
        // Find DigitalPersona device
        val device = findDevice()
            ?: return CaptureResult.Error(FingerError.DEVICE_NOT_FOUND)

        // Request permission if needed
        // TODO: Implement USB permission request

        // Open via native
        val result = LibfprintNative.openDevice(device)
        if (result) {
            isDeviceOpen = true
            return CaptureResult.Success(ByteArray(0)) // Placeholder
        } else {
            return CaptureResult.Error(FingerError.DEVICE_INIT_FAILED)
        }
    }

    /**
     * Close USB device.
     */
    fun closeDevice() {
        if (isDeviceOpen) {
            LibfprintNative.closeDevice()
            isDeviceOpen = false
        }
    }

    /**
     * Find DigitalPersona USB device.
     */
    private fun findDevice(): UsbDevice? {
        val deviceList = usbManager.deviceList
        for (device in deviceList.values) {
            // DigitalPersona VID/PID: 0x05BA/0x000A or 0x05BA/0x0034
            if (device.vendorId == 0x05BA && 
                (device.productId == 0x000A || device.productId == 0x0034)) {
                return device
            }
        }
        return null
    }
}


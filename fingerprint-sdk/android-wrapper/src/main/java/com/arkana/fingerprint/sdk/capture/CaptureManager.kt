package com.arkana.fingerprint.sdk.capture

import android.content.Context
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbManager
import android.util.Log
import com.arkana.fingerprint.sdk.model.CaptureResult
import com.arkana.fingerprint.sdk.util.FingerError
import com.arkana.fingerprint.sdk.capture.native.LibfprintNative
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlin.coroutines.resume

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
    private var usbConnection: UsbDeviceConnection? = null

    companion object {
        private const val TAG = "CaptureManager"
    }

    /**
     * Capture fingerprint once (single capture).
     * 
     * @param timeoutMs Timeout in milliseconds
     * @return CaptureResult
     */
    suspend fun captureOnce(timeoutMs: Long): CaptureResult {
        Log.d(TAG, "captureOnce: timeoutMs=$timeoutMs, isDeviceOpen=$isDeviceOpen")
        
        // Ensure device is open
        if (!isDeviceOpen) {
            val openResult = openDevice()
            if (openResult is CaptureResult.Error) {
                Log.e(TAG, "Failed to open device: ${openResult.error}")
                return openResult
            }
        }

        // Capture via native
        Log.d(TAG, "Calling native capture...")
        return LibfprintNative.capture(timeoutMs)
    }
    
    /**
     * Enroll fingerprint (full enrollment with multiple scans).
     * This uses fp_device_enroll_sync which performs all required scans internally.
     * 
     * @return CaptureResult with FP3 template
     */
    suspend fun enroll(): CaptureResult {
        Log.d(TAG, "enroll: isDeviceOpen=$isDeviceOpen")
        
        // Ensure device is open
        if (!isDeviceOpen) {
            val openResult = openDevice()
            if (openResult is CaptureResult.Error) {
                Log.e(TAG, "Failed to open device: ${openResult.error}")
                return openResult
            }
        }

        // Enroll via native (performs all required scans)
        Log.d(TAG, "Calling native enroll...")
        return LibfprintNative.enroll()
    }

    /**
     * Open USB device (internal).
     */
    private suspend fun openDevice(): CaptureResult {
        Log.d(TAG, "openDevice: Starting...")
        
        // Find DigitalPersona device
        val device = findDevice()
        if (device == null) {
            Log.e(TAG, "Device not found")
            return CaptureResult.Error(FingerError.DEVICE_NOT_FOUND)
        }

        Log.d(TAG, "Found device: VID=${device.vendorId.toString(16)}, PID=${device.productId.toString(16)}")

        // Check permission
        if (!usbManager.hasPermission(device)) {
            Log.w(TAG, "USB permission not granted, requesting...")
            return CaptureResult.Error(FingerError.DEVICE_PERMISSION_DENIED)
        }

        Log.d(TAG, "USB permission granted, opening connection...")
        
        // Open USB connection
        val connection = usbManager.openDevice(device)
        if (connection == null) {
            Log.e(TAG, "Failed to open USB connection")
            return CaptureResult.Error(FingerError.DEVICE_INIT_FAILED)
        }

        usbConnection = connection
        val fd = connection.fileDescriptor
        Log.d(TAG, "USB connection opened, file descriptor: $fd")

        if (fd < 0) {
            Log.e(TAG, "Invalid file descriptor: $fd")
            connection.close()
            usbConnection = null
            return CaptureResult.Error(FingerError.DEVICE_INIT_FAILED)
        }

        // Open via native with file descriptor
        val result = LibfprintNative.openDevice(fd)
        if (result) {
            isDeviceOpen = true
            Log.d(TAG, "Device opened successfully")
            return CaptureResult.Success(ByteArray(0)) // Placeholder
        } else {
            Log.e(TAG, "Native openDevice failed")
            connection.close()
            usbConnection = null
            return CaptureResult.Error(FingerError.DEVICE_INIT_FAILED)
        }
    }

    /**
     * Check if device is open.
     */
    fun isDeviceOpen(): Boolean {
        return isDeviceOpen
    }

    /**
     * Open device (public method for SDK to ensure device is open).
     */
    suspend fun ensureDeviceOpen(): CaptureResult {
        if (isDeviceOpen) {
            Log.d(TAG, "Device already open")
            return CaptureResult.Success(ByteArray(0))
        }
        return openDevice()
    }

    /**
     * Close USB device.
     */
    fun closeDevice() {
        Log.d(TAG, "closeDevice: isDeviceOpen=$isDeviceOpen")
        if (isDeviceOpen) {
            LibfprintNative.closeDevice()
            usbConnection?.close()
            usbConnection = null
            isDeviceOpen = false
            Log.d(TAG, "Device closed")
        }
    }

    /**
     * Find DigitalPersona USB device.
     */
    private fun findDevice(): UsbDevice? {
        val deviceList = usbManager.deviceList
        Log.d(TAG, "Searching for device in ${deviceList.size} devices")
        
        for (device in deviceList.values) {
            Log.d(TAG, "Checking device: VID=${device.vendorId.toString(16)}, PID=${device.productId.toString(16)}")
            // DigitalPersona VID/PID: 0x05BA/0x000A or 0x05BA/0x0034
            if (device.vendorId == 0x05BA && 
                (device.productId == 0x000A || device.productId == 0x0034)) {
                Log.d(TAG, "Found DigitalPersona device!")
                return device
            }
        }
        
        Log.w(TAG, "DigitalPersona device not found")
        return null
    }
}


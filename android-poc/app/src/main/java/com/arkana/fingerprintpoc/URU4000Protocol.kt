package com.arkana.fingerprintpoc

import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbEndpoint
import android.hardware.usb.UsbInterface
import android.util.Log

/**
 * DigitalPersona U.are.U 4000 USB Protocol Implementation
 * Based on libfprint uru4000 driver
 */
object URU4000Protocol {
    private const val TAG = "URU4000Protocol"
    
    // USB Constants
    private const val USB_RQ = 0x04
    private const val USB_REQUEST_TYPE_VENDOR = 0x40 // LIBUSB_REQUEST_TYPE_VENDOR
    private const val USB_ENDPOINT_IN = 0x80
    private const val USB_ENDPOINT_OUT = 0x00
    
    // Register addresses
    private const val REG_HWSTAT = 0x07
    private const val REG_MODE = 0x4e
    private const val REG_SCRAMBLE_DATA_INDEX = 0x33
    private const val REG_SCRAMBLE_DATA_KEY = 0x34
    
    // Mode values
    private const val MODE_INIT = 0x00
    private const val MODE_AWAIT_FINGER_ON = 0x10
    private const val MODE_CAPTURE = 0x20
    private const val MODE_READY = 0x80
    private const val MODE_OFF = 0x70
    
    // Image dimensions (U.are.U 4000)
    private const val IMAGE_WIDTH = 384
    private const val IMAGE_HEIGHT = 290
    
    // Timeouts
    private const val CTRL_TIMEOUT = 5000
    private const val BULK_TIMEOUT = 5000
    
    /**
     * Initialize device - set mode and enable scanning
     */
    fun initializeDevice(connection: UsbDeviceConnection, usbInterface: UsbInterface): Boolean {
        try {
            // Claim interface
            if (!connection.claimInterface(usbInterface, true)) {
                Log.e(TAG, "Failed to claim interface")
                return false
            }
            
            // Read HWSTAT register
            val hwstat = readRegister(connection, REG_HWSTAT)
            if (hwstat == null) {
                Log.e(TAG, "Failed to read HWSTAT")
                return false
            }
            
            Log.d(TAG, "HWSTAT: 0x${hwstat.toString(16)}")
            
            // Set mode to INIT
            if (!writeRegister(connection, REG_MODE, MODE_INIT.toByte())) {
                Log.e(TAG, "Failed to set MODE_INIT")
                return false
            }
            
            Thread.sleep(100) // Allow device to process
            
            // Set mode to AWAIT_FINGER_ON
            if (!writeRegister(connection, REG_MODE, MODE_AWAIT_FINGER_ON.toByte())) {
                Log.e(TAG, "Failed to set MODE_AWAIT_FINGER_ON")
                return false
            }
            
            Log.i(TAG, "Device initialized successfully")
            return true
            
        } catch (e: Exception) {
            Log.e(TAG, "Error initializing device: ${e.message}", e)
            return false
        }
    }
    
    /**
     * Read a register from device
     */
    private fun readRegister(connection: UsbDeviceConnection, register: Int): Byte? {
        val buffer = ByteArray(16)
        val result = connection.controlTransfer(
            USB_REQUEST_TYPE_VENDOR or USB_ENDPOINT_IN, // requestType
            USB_RQ, // request
            register, // value
            0, // index
            buffer, // buffer
            buffer.size, // length
            CTRL_TIMEOUT // timeout
        )
        
        if (result < 0) {
            Log.e(TAG, "Failed to read register 0x${register.toString(16)}: result=$result")
            return null
        }
        
        return buffer[0]
    }
    
    /**
     * Write a register to device
     */
    private fun writeRegister(connection: UsbDeviceConnection, register: Int, value: Byte): Boolean {
        val buffer = byteArrayOf(value)
        val result = connection.controlTransfer(
            USB_REQUEST_TYPE_VENDOR or USB_ENDPOINT_OUT, // requestType
            USB_RQ, // request
            register, // value
            0, // index
            buffer, // buffer
            buffer.size, // length
            CTRL_TIMEOUT // timeout
        )
        
        if (result < 0) {
            Log.e(TAG, "Failed to write register 0x${register.toString(16)}: result=$result")
            return false
        }
        
        return true
    }
    
    /**
     * Capture fingerprint image from device
     */
    fun captureImage(
        connection: UsbDeviceConnection,
        usbInterface: UsbInterface,
        bulkEndpoint: UsbEndpoint?
    ): ByteArray? {
        try {
            if (bulkEndpoint == null) {
                Log.e(TAG, "Bulk endpoint not found")
                return null
            }
            
            // Wait for finger first - set mode to AWAIT_FINGER_ON
            if (!writeRegister(connection, REG_MODE, MODE_AWAIT_FINGER_ON.toByte())) {
                Log.e(TAG, "Failed to set MODE_AWAIT_FINGER_ON")
                return null
            }
            
            Log.d(TAG, "Waiting for finger on sensor...")
            Thread.sleep(1000) // Wait for finger to be placed
            
            // Now set mode to CAPTURE
            if (!writeRegister(connection, REG_MODE, MODE_CAPTURE.toByte())) {
                Log.e(TAG, "Failed to set MODE_CAPTURE")
                return null
            }
            
            // Wait a bit for capture to start
            Thread.sleep(200)
            
            // Read image data from bulk endpoint
            // Size should match struct uru4k_image: metadata + image data
            // Metadata: 4 + 2 + 1 + 9 + (15*2) + 18 = 64 bytes
            // Image data: up to IMAGE_HEIGHT * IMAGE_WIDTH = 111360 bytes
            // Total: ~111424 bytes, but actual may vary
            val bufferSize = 64 + (IMAGE_WIDTH * IMAGE_HEIGHT) // Full struct size
            val buffer = ByteArray(bufferSize)
            val result = connection.bulkTransfer(
                bulkEndpoint,
                buffer,
                buffer.size,
                BULK_TIMEOUT
            )
            
            if (result < 0) {
                Log.e(TAG, "Failed to read image data: result=$result")
                return null
            }
            
            Log.i(TAG, "Captured ${result} bytes of raw image data (struct uru4k_image)")
            
            // IMPORTANT: Release device after capture to stop LED blinking
            // Set mode to READY or OFF to deactivate sensor
            if (!writeRegister(connection, REG_MODE, MODE_READY.toByte())) {
                Log.w(TAG, "Failed to set MODE_READY after capture (non-critical)")
            } else {
                Log.d(TAG, "Device released - set to MODE_READY")
            }
            
            // Return full captured data for decoding
            return if (result > 0) {
                buffer.copyOf(result)
            } else {
                null
            }
            
        } catch (e: Exception) {
            Log.e(TAG, "Error capturing image: ${e.message}", e)
            
            // Try to release device even on error
            try {
                writeRegister(connection, REG_MODE, MODE_READY.toByte())
                Log.d(TAG, "Device released after error")
            } catch (releaseError: Exception) {
                Log.w(TAG, "Failed to release device after error: ${releaseError.message}")
            }
            
            return null
        }
    }
    
    /**
     * Release device - reset to ready state to stop LED blinking
     * Call this after capture to properly release the device
     */
    fun releaseDevice(connection: UsbDeviceConnection): Boolean {
        return try {
            Log.d(TAG, "Releasing device - setting to MODE_READY")
            writeRegister(connection, REG_MODE, MODE_READY.toByte())
        } catch (e: Exception) {
            Log.e(TAG, "Error releasing device: ${e.message}", e)
            false
        }
    }
}


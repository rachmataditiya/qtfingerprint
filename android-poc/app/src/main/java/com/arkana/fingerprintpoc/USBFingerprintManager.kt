package com.arkana.fingerprintpoc

import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbDeviceConnection
import android.hardware.usb.UsbEndpoint
import android.hardware.usb.UsbInterface
import android.hardware.usb.UsbManager
import android.hardware.usb.UsbRequest
import kotlinx.coroutines.delay
import java.nio.ByteBuffer
import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.core.content.ContextCompat
import androidx.fragment.app.FragmentActivity
import com.arkana.libdigitalpersona.FingerprintJNI
import com.arkana.fingerprintpoc.URU4000Protocol
import com.arkana.fingerprintpoc.URU4000ImageDecoder

/**
 * USB Fingerprint Manager for DigitalPersona U.are.U 4000
 * Uses Android USB Host API to communicate with USB fingerprint reader
 */
class USBFingerprintManager(private val context: Context) {
    
    companion object {
        private const val TAG = "USBFingerprintManager"
        private const val ACTION_USB_PERMISSION = "com.arkana.fingerprintpoc.USB_PERMISSION"
        
        // DigitalPersona U.are.U 4000
        private const val VENDOR_ID = 0x05BA  // 1466
        private const val PRODUCT_ID = 0x000A // 10
    }
    
    private val usbManager: UsbManager = context.getSystemService(Context.USB_SERVICE) as UsbManager
    private var usbDevice: UsbDevice? = null
    private var usbConnection: UsbDeviceConnection? = null
    private var permissionIntent: PendingIntent? = null
    private var activity: FragmentActivity? = null
    
    // libdigitalpersona JNI instance
    private var libdigitalpersonaInitialized = false
    
    private val usbReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            when (intent.action) {
                ACTION_USB_PERMISSION -> {
                    synchronized(this) {
                        val device: UsbDevice? = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
                        if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                            device?.let {
                                Log.d(TAG, "USB permission granted for device: ${it.deviceName}")
                                usbDevice = it
                                openUsbConnection()
                            }
                        } else {
                            Log.d(TAG, "USB permission denied for device: ${device?.deviceName}")
                        }
                    }
                }
                UsbManager.ACTION_USB_DEVICE_ATTACHED -> {
                    val device: UsbDevice? = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
                    device?.let {
                        Log.d(TAG, "USB device attached: ${it.deviceName}, VID: 0x${it.vendorId.toString(16)}, PID: 0x${it.productId.toString(16)}")
                        if (it.vendorId == VENDOR_ID && it.productId == PRODUCT_ID) {
                            Log.i(TAG, "DigitalPersona U.are.U 4000 detected!")
                            usbDevice = it
                            // Request permission if needed
                            activity?.let { act ->
                                requestPermission(act, it)
                            }
                        }
                    }
                }
                UsbManager.ACTION_USB_DEVICE_DETACHED -> {
                    val device: UsbDevice? = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE)
                    device?.let {
                        Log.d(TAG, "USB device detached: ${it.deviceName}")
                        if (usbDevice?.deviceId == it.deviceId) {
                            usbDevice = null
                            Log.i(TAG, "DigitalPersona device disconnected")
                        }
                    }
                }
            }
        }
    }
    
    fun initialize(activity: FragmentActivity): Boolean {
        this.activity = activity
        
        // Initialize libdigitalpersona JNI
        // Skip for now - libfprint not compiled for Android yet
        // Will use direct USB communication instead
        libdigitalpersonaInitialized = false
        Log.d(TAG, "libdigitalpersona JNI skipped, using direct USB communication")
        
        // Register USB receiver
        val filter = IntentFilter().apply {
            addAction(ACTION_USB_PERMISSION)
            addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED)
            addAction(UsbManager.ACTION_USB_DEVICE_DETACHED)
        }
        permissionIntent = PendingIntent.getBroadcast(
            context, 0, Intent(ACTION_USB_PERMISSION),
            PendingIntent.FLAG_IMMUTABLE
        )
        // Android 13+ (API 33+) requires RECEIVER_EXPORTED or RECEIVER_NOT_EXPORTED flag
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            context.registerReceiver(usbReceiver, filter, Context.RECEIVER_NOT_EXPORTED)
        } else {
            @Suppress("DEPRECATION")
            context.registerReceiver(usbReceiver, filter)
        }
        
        // Check for already connected device
        findConnectedDevice()
        
        return usbDevice != null
    }
    
    private fun findConnectedDevice() {
        val deviceList = usbManager.deviceList
        Log.d(TAG, "Checking ${deviceList.size} USB devices...")
        
        for (device in deviceList.values) {
            Log.d(TAG, "Found USB device: ${device.deviceName}, VID: 0x${device.vendorId.toString(16)}, PID: 0x${device.productId.toString(16)}")
            if (device.vendorId == VENDOR_ID && device.productId == PRODUCT_ID) {
                Log.i(TAG, "DigitalPersona U.are.U 4000 found!")
                usbDevice = device
                
                // Request permission if needed
                if (!usbManager.hasPermission(device)) {
                    activity?.let {
                        requestPermission(it, device)
                    }
                } else {
                    Log.i(TAG, "USB permission already granted")
                    openUsbConnection()
                }
                return
            }
        }
        
        Log.w(TAG, "DigitalPersona U.are.U 4000 not found")
    }
    
    private fun requestPermission(activity: FragmentActivity, device: UsbDevice) {
        if (usbManager.hasPermission(device)) {
            Log.d(TAG, "USB permission already granted")
            openUsbConnection()
            return
        }
        
        Log.d(TAG, "Requesting USB permission for device: ${device.deviceName}")
        permissionIntent?.let {
            usbManager.requestPermission(device, it)
        }
    }
    
    private fun openUsbConnection(): Boolean {
        val device = usbDevice ?: return false
        
        if (!usbManager.hasPermission(device)) {
            Log.w(TAG, "USB permission not granted yet")
            return false
        }
        
        try {
            usbConnection = usbManager.openDevice(device)
            if (usbConnection != null) {
                Log.i(TAG, "USB connection opened successfully")
                
                // Claim first interface (DigitalPersona devices typically use interface 0)
                val interfaceCount = device.interfaceCount
                Log.d(TAG, "Device has $interfaceCount interfaces")
                
                for (i in 0 until interfaceCount) {
                    val usbInterface = device.getInterface(i)
                    Log.d(TAG, "Interface $i: Class=${usbInterface.interfaceClass}, Subclass=${usbInterface.interfaceSubclass}, Protocol=${usbInterface.interfaceProtocol}")
                    
                    // Try to claim this interface
                    if (usbConnection?.claimInterface(usbInterface, true) == true) {
                        Log.i(TAG, "Claimed interface $i")
                        
                        // Log endpoints
                        for (j in 0 until usbInterface.endpointCount) {
                            val endpoint = usbInterface.getEndpoint(j)
                            Log.d(TAG, "  Endpoint $j: Type=${endpoint.type}, Address=${endpoint.address}, Direction=${if (endpoint.direction == 0) "OUT" else "IN"}")
                        }
                    } else {
                        Log.w(TAG, "Failed to claim interface $i")
                    }
                }
                
                return true
            } else {
                Log.e(TAG, "Failed to open USB device")
                return false
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error opening USB connection: ${e.message}", e)
            return false
        }
    }
    
    fun getDeviceCount(): Int {
        return if (usbDevice != null) 1 else 0
    }
    
    fun isDeviceConnected(): Boolean {
        return usbDevice != null && usbManager.hasPermission(usbDevice!!)
    }
    
    fun cancel() {
        // Cancel any ongoing operations
        Log.d(TAG, "Cancel called")
        // TODO: Cancel USB communication if in progress
    }
    
    fun cleanup() {
        try {
            context.unregisterReceiver(usbReceiver)
        } catch (e: Exception) {
            Log.w(TAG, "Error unregistering receiver: ${e.message}")
        }
        
        // Cleanup libdigitalpersona
        if (libdigitalpersonaInitialized) {
            try {
                FingerprintJNI.cleanup()
            } catch (e: Exception) {
                Log.w(TAG, "Error cleaning up libdigitalpersona: ${e.message}")
            }
            libdigitalpersonaInitialized = false
        }
        
        permissionIntent = null
        activity = null
        usbDevice = null
        usbConnection?.close()
        usbConnection = null
    }
    
    suspend fun captureFingerprint(): ByteArray? {
        if (!isDeviceConnected()) {
            Log.e(TAG, "USB fingerprint reader not connected")
            return null
        }
        
        val device = usbDevice ?: return null
        
        Log.i(TAG, "Starting fingerprint capture...")
        Log.d(TAG, "Device: ${device.deviceName}, VID: 0x${device.vendorId.toString(16)}, PID: 0x${device.productId.toString(16)}")
        
        // Try libdigitalpersona JNI capture first
        if (libdigitalpersonaInitialized) {
            try {
                Log.d(TAG, "Attempting capture via libdigitalpersona JNI...")
                val template = FingerprintJNI.captureFingerprint()
                if (template != null && template.isNotEmpty()) {
                    Log.i(TAG, "Fingerprint captured via libdigitalpersona: ${template.size} bytes")
                    return template
                } else {
                    Log.w(TAG, "libdigitalpersona capture returned null/empty")
                }
            } catch (e: Exception) {
                Log.w(TAG, "libdigitalpersona capture failed: ${e.message}", e)
            }
        }
        
        // Fallback: Try direct USB communication
        val connection = usbConnection ?: run {
            Log.w(TAG, "USB connection not open, attempting to open...")
            if (!openUsbConnection()) {
                Log.e(TAG, "Failed to open USB connection")
                return generateMockTemplate() // Use mock for testing
            }
            usbConnection
        } ?: return generateMockTemplate()
        
        try {
            // Attempt real USB capture
            val template = attemptRealCapture(device, connection)
            if (template != null) {
                Log.i(TAG, "Fingerprint captured via USB: ${template.size} bytes")
                return template
            }
        } catch (e: Exception) {
            Log.w(TAG, "USB capture failed: ${e.message}", e)
        }
        
        // Final fallback: mock template for API testing
        Log.i(TAG, "Using mock template for API flow testing")
        return generateMockTemplate()
    }
    
    private suspend fun attemptRealCapture(device: UsbDevice, connection: UsbDeviceConnection): ByteArray? {
        try {
            Log.d(TAG, "Attempting real USB capture using URU4000 protocol...")
            
            // Find bulk endpoint for image data
            val usbInterface = device.getInterface(0)
            var bulkEndpoint: android.hardware.usb.UsbEndpoint? = null
            
            for (i in 0 until usbInterface.endpointCount) {
                val ep = usbInterface.getEndpoint(i)
                if (ep.type == android.hardware.usb.UsbConstants.USB_ENDPOINT_XFER_BULK) {
                    bulkEndpoint = ep
                    Log.d(TAG, "Found bulk endpoint: address=0x${ep.address.toString(16)}")
                    break
                }
            }
            
            if (bulkEndpoint == null) {
                Log.w(TAG, "Bulk endpoint not found, cannot capture image")
                return null
            }
            
            // Initialize device using protocol
            if (!URU4000Protocol.initializeDevice(connection, usbInterface)) {
                Log.e(TAG, "Failed to initialize device")
                return null
            }
            
            Log.i(TAG, "Waiting for finger on sensor...")
            delay(2000) // Wait for finger
            
            // Capture image
            val imageData = URU4000Protocol.captureImage(connection, usbInterface, bulkEndpoint)
            
            // IMPORTANT: Ensure device is released even if capture fails
            // The captureImage() function already releases device, but we add extra safety here
            try {
                URU4000Protocol.releaseDevice(connection)
            } catch (e: Exception) {
                Log.w(TAG, "Failed to release device (non-critical): ${e.message}")
            }
            
            if (imageData != null && imageData.isNotEmpty()) {
                Log.i(TAG, "Raw image captured: ${imageData.size} bytes")
                
                // Decode raw bulk transfer data to image pixels
                val decodedImage = URU4000ImageDecoder.decodeImage(imageData)
                if (decodedImage != null && decodedImage.isNotEmpty()) {
                    Log.i(TAG, "Image decoded: ${decodedImage.size} bytes (${URU4000ImageDecoder.IMAGE_WIDTH}x${URU4000ImageDecoder.IMAGE_HEIGHT})")
                    
                    // Return decoded image data (raw grayscale pixels, 384x290 = 111360 bytes)
                    // This format is correct for matching with FP3 templates from database
                    // Server will use image_compare or minutiae_match to compare with stored templates
                    return decodedImage
                } else {
                    Log.w(TAG, "Failed to decode image, returning raw data")
                    return imageData
                }
            } else {
                Log.w(TAG, "No image data captured")
                return null
            }
            
        } catch (e: Exception) {
            Log.e(TAG, "Error in real USB capture: ${e.message}", e)
            
            // Ensure device is released on error
            try {
                val connection = usbConnection
                if (connection != null) {
                    URU4000Protocol.releaseDevice(connection)
                }
            } catch (releaseError: Exception) {
                Log.w(TAG, "Failed to release device after error: ${releaseError.message}")
            }
            
            return null
        }
    }
    
    private fun generateMockTemplate(): ByteArray {
        // Generate a mock fingerprint template for API testing
        // This simulates a real template structure
        // In production, this should be replaced with actual capture
        
        Log.d(TAG, "Generating mock fingerprint template for testing")
        
        // Create a mock template (minimal valid structure)
        // Format: simple binary data that represents a fingerprint template
        val mockTemplate = ByteArray(256) { (it * 7 + 13).toByte() } // Simple pattern
        
        // Add some header/metadata to make it look more realistic
        mockTemplate[0] = 0x46.toByte() // 'F'
        mockTemplate[1] = 0x50.toByte() // 'P'
        mockTemplate[2] = 0x54.toByte() // 'T'
        mockTemplate[3] = 0x01.toByte() // Version
        
        Log.d(TAG, "Mock template generated: ${mockTemplate.size} bytes")
        return mockTemplate
    }
    
    suspend fun verifyFingerprint(userId: Int): Pair<Boolean, Int> {
        if (!isDeviceConnected()) {
            Log.e(TAG, "USB fingerprint reader not connected")
            return Pair(false, 0)
        }
        
        // TODO: This will be implemented to:
        // 1. Capture fingerprint from USB reader
        // 2. Send template to REST API /users/{userId}/verify endpoint
        // 3. Return result from API
        
        // For now, return no match - will be implemented with API integration
        Log.w(TAG, "verifyFingerprint not yet implemented - requires USB capture + API integration")
        return Pair(false, 0)
    }
    
    suspend fun identifyUser(userIds: List<Int>): Pair<Int, Int> {
        if (!isDeviceConnected()) {
            Log.e(TAG, "USB fingerprint reader not connected")
            return Pair(-1, 0)
        }
        
        // TODO: This will be implemented to:
        // 1. Capture fingerprint from USB reader
        // 2. Send template to REST API /identify endpoint
        // 3. Return result from API
        
        // For now, return no match - will be implemented with API integration
        Log.w(TAG, "identifyUser not yet implemented - requires USB capture + API integration")
        return Pair(-1, 0)
    }
}


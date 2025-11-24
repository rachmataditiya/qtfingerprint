package com.arkana.fingerprintsdk.exceptions

/**
 * Base exception for SDK errors.
 */
open class SDKException(message: String, cause: Throwable? = null) : Exception(message, cause)

/**
 * Thrown when SDK is not initialized.
 */
class SDKNotInitializedException : SDKException("SDK not initialized. Call FingerprintSDK.initialize() first.")

/**
 * Thrown when USB device is not found or not connected.
 */
class USBDeviceNotFoundException : SDKException("USB device not found or not connected.")

/**
 * Thrown when USB permission is denied.
 */
class USBPermissionDeniedException : SDKException("USB permission denied.")

/**
 * Thrown when device initialization fails.
 */
class DeviceInitializationException(message: String) : SDKException("Device initialization failed: $message")


package com.arkana.fingerprint.sdk.util

/**
 * Fingerprint SDK error enumeration.
 */
enum class FingerError {
    // Initialization errors
    SDK_NOT_INITIALIZED,
    DEVICE_NOT_FOUND,
    DEVICE_INIT_FAILED,
    DEVICE_PERMISSION_DENIED,
    
    // USB errors
    USB_PERMISSION_DENIED,
    USB_DEVICE_NOT_FOUND,
    USB_ERROR,
    
    // Capture errors
    CAPTURE_TIMEOUT,
    CAPTURE_FAILED,
    CAPTURE_CANCELLED,
    
    // Matching errors
    MATCHING_FAILED,
    VERIFICATION_FAILED,
    IDENTIFICATION_FAILED,
    NO_TEMPLATES,
    
    // Backend errors
    BACKEND_ERROR,
    NETWORK_ERROR,
    TEMPLATE_NOT_FOUND,
    
    // Cache errors
    CACHE_ERROR,
    DECRYPTION_FAILED,
    
    // Unknown
    UNKNOWN_ERROR
}


package com.arkana.fingerprint.sdk.config

/**
 * SDK configuration.
 * 
 * @param backendUrl Backend API URL
 * @param enrollmentScans Number of scans required for enrollment (default: 5)
 * @param matchThreshold Minimum score for successful match (0.0-1.0, default: 0.6)
 * @param timeoutMs Operation timeout in milliseconds (default: 30000)
 * @param enableCache Enable secure cache for templates (default: true)
 */
data class FingerprintSdkConfig(
    val backendUrl: String,
    val enrollmentScans: Int = 5,
    val matchThreshold: Float = 0.6f,
    val timeoutMs: Long = 30000,
    val enableCache: Boolean = true
) {
    init {
        require(enrollmentScans > 0) { "enrollmentScans must be > 0" }
        require(matchThreshold in 0.0f..1.0f) { "matchThreshold must be between 0.0 and 1.0" }
        require(timeoutMs > 0) { "timeoutMs must be > 0" }
    }
}


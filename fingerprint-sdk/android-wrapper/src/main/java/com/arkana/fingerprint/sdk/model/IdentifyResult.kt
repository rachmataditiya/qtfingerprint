package com.arkana.fingerprint.sdk.model

import com.arkana.fingerprint.sdk.util.FingerError

/**
 * Result of identification operation.
 */
sealed class IdentifyResult {
    data class Success(val userId: Int, val score: Float) : IdentifyResult()
    data class Error(val error: FingerError) : IdentifyResult()
}


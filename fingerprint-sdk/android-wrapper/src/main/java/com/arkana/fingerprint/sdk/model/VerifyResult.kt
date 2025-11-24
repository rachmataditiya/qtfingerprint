package com.arkana.fingerprint.sdk.model

import com.arkana.fingerprint.sdk.util.FingerError

/**
 * Result of verification operation.
 */
sealed class VerifyResult {
    data class Success(val userId: Int, val score: Float) : VerifyResult()
    data class Error(val error: FingerError) : VerifyResult()
}


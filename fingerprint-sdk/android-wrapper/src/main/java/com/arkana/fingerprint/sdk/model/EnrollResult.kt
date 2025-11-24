package com.arkana.fingerprint.sdk.model

import com.arkana.fingerprint.sdk.util.FingerError

/**
 * Result of enrollment operation.
 */
sealed class EnrollResult {
    data class Success(val userId: Int) : EnrollResult()
    data class Error(val error: FingerError) : EnrollResult()
}


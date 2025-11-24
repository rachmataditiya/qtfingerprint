package com.arkana.fingerprint.sdk.model

import com.arkana.fingerprint.sdk.util.FingerError

/**
 * Result of fingerprint capture operation.
 */
sealed class CaptureResult {
    data class Success(val template: ByteArray) : CaptureResult()
    data class Error(val error: FingerError) : CaptureResult()
}


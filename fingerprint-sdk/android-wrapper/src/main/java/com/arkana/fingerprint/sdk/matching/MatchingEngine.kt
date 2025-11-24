package com.arkana.fingerprint.sdk.matching

import com.arkana.fingerprint.sdk.capture.native.LibfprintNative

/**
 * Matching engine for fingerprint templates.
 * 
 * Uses libfprint match() function with scoring 0.0-1.0.
 */
class MatchingEngine {
    /**
     * Create template from multiple scans.
     * 
     * @param scans List of fingerprint scans
     * @return Combined template
     */
    fun createTemplate(scans: List<ByteArray>): ByteArray {
        // TODO: Implement template creation from multiple scans
        // For now, use the first scan as template
        return scans.firstOrNull() ?: ByteArray(0)
    }

    /**
     * Match two templates.
     * 
     * @param template1 First template
     * @param template2 Second template
     * @return Match score (0.0-1.0)
     */
    fun match(template1: ByteArray, template2: ByteArray): Float {
        return LibfprintNative.match(template1, template2)
    }
}


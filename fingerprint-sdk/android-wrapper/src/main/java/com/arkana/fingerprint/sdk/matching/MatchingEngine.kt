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
     * matchWithTemplate will capture current fingerprint and match with stored template in one step.
     * 
     * @param template1 First template (ignored, native captures live)
     * @param template2 Second template (stored template from database)
     * @return Match score (0.0-1.0)
     */
    fun match(template1: ByteArray, template2: ByteArray): Float {
        return LibfprintNative.match(template1, template2)
    }
    
    /**
     * Identify user from multiple templates (1:N matching).
     * identifyUser will capture current fingerprint and match against all templates in one step.
     * 
     * @param templates List of TemplateInfo (includes all fingers per user)
     * @return Pair of (matchedIndex, score), or (-1, 0.0f) if no match
     */
    fun identify(templates: List<com.arkana.fingerprint.sdk.backend.TemplateInfo>): Pair<Int, Float> {
        if (templates.isEmpty()) {
            return Pair(-1, 0.0f)
        }
        
        // Build arrays for JNI call (one entry per template, not per user)
        val userIds = templates.map { it.userId }.toIntArray()
        val fingers = templates.map { it.finger }.toTypedArray()
        val templateArray = templates.map { it.template }.toTypedArray()
        val scoreOut = IntArray(1)
        val matchedIndexOut = IntArray(1)
        
        val matchedIndex = LibfprintNative.identify(userIds, fingers, templateArray, scoreOut, matchedIndexOut)
        val score = scoreOut[0] / 100.0f
        
        // Return matched index directly so we can get full template info including userName
        return Pair(matchedIndexOut[0], score)
    }
}

